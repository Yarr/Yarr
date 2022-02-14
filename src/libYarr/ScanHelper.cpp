#include "ScanHelper.h"

#include <iostream>
#include <exception>
#include <iomanip>
#include <numeric>

#include "AllAnalyses.h"
#include "AllChips.h"
#include "AllHistogrammers.h"
#include "AllHwControllers.h"
#include "AllProcessors.h"
#include "AllStdActions.h"

#include "AnalysisAlgorithm.h"
#include "HistogramAlgorithm.h"
#include "StdHistogrammer.h" // needed for special handling of DataArchiver
#include "ScanFactory.h"

#include "logging.h"
#include "LoggingConfig.h"

#include "ScanOpts.h"

namespace {
    auto shlog = logging::make_log("ScanHelper");
    auto bhlog = logging::make_log("ScanBuildHistogrammers");
    auto balog = logging::make_log("ScanBuildAnalyses");
}

namespace ScanHelper {

    unsigned newRunCounter() {
        unsigned runCounter = 0;

        std::string home;
        if(getenv("HOME")) {
            home = getenv("HOME");
        } else {
            shlog->error("HOME not set, using local directory for configuration");
            home = ".";
        }
        std::string config_dir = home + "/.yarr";

        // Load run counter
        std::string mkdir_command = "mkdir -p " + config_dir;
        if (system(mkdir_command.c_str()) < 0) {
            shlog->error("Failed to create dir for run counter: ~/.yarr!");
        }

        std::string run_counter_file_name = config_dir + "/runCounter";
        std::fstream iF(run_counter_file_name.c_str(), std::ios::in);
        if (iF) {
            iF >> runCounter;
            runCounter += 1;
        } else {
            runCounter = 1;
        }
        iF.close();

        std::fstream oF((home + "/.yarr/runCounter").c_str(), std::ios::out);
        if(!oF) {
            shlog->error("Could not increment run counter in file");
        }
        oF << runCounter << std::endl;
        oF.close();

        return runCounter;
    }

    // Open file and parse into json object
    json openJsonFile(std::string filepath) {
        std::ifstream file(filepath);
        if (!file) {
            throw std::runtime_error("could not open file");
        }
        json j;
        try {
            j = json::parse(file);
        } catch (json::parse_error &e) {
            throw std::runtime_error(e.what());
        }
        file.close();
        // variant produces null for some parse errors
        if(j.is_null()) {
            throw std::runtime_error("Parsing json file produced null");
        }
        return j;
    }
    // Load controller config and return fully loaded object
    std::unique_ptr<HwController> loadController(const json &ctrlCfg) {
        std::unique_ptr<HwController> hwCtrl = nullptr;

        shlog->info("Loading controller ...");

        // Open controller config file
        std::string controller = ctrlCfg["ctrlCfg"]["type"];

        hwCtrl = StdDict::getHwController(controller);

        if(hwCtrl) {
            shlog->info("Found controller of type: {}", controller);
            shlog->info("... loading controler config:");
            std::stringstream ss;
            ss << ctrlCfg["ctrlCfg"]["cfg"];
            std::string line;
            while (std::getline(ss, line)) shlog->info("~~~ {}", line); //yes overkill, i know ..

            hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
        } else {
            shlog->critical("Unknown config type: {}",  std::string(ctrlCfg["ctrlCfg"]["type"]));
            shlog->warn("Known HW controllers:");
            for(auto &h: StdDict::listHwControllers()) {
                shlog->warn("  {}", h);
            }
            shlog->critical("Aborting!");
            throw(std::runtime_error("loadController failure"));
        }
        return hwCtrl;
    }


    void buildConfig(json &j) {

    }

    // Load connectivyt and load chips into bookkeeper
    std::string loadChips(const json &config, Bookkeeper &bookie, HwController *hwCtrl, std::map<FrontEnd*, std::string> &feCfgMap, std::string &outputDir) {
        std::string chipType;
        if (!config.contains("chipType") || !config.contains("chips")) {
            shlog->error("Invalid config, chip type or chips not specified!");
            throw(std::runtime_error("loadChips failure"));
        } else {
            chipType = config["chipType"];
            shlog->info("Chip type: {}", chipType);
            shlog->info("Chip count {}", config["chips"].size());
            // Loop over chips
            for (unsigned i=0; i<config["chips"].size(); i++) {
                shlog->info("Loading chip #{}", i);
                json chip = config["chips"][i];
                std::string chipConfigPath = chip["config"];
                if (chip["enable"] == 0) {
                    shlog->warn(" ... chip not enabled, skipping!");
                } else {
                    // TODO should be a shared pointer
                    bookie.addFe(StdDict::getFrontEnd(chipType).release(), chip["tx"], chip["rx"]);
                    bookie.getLastFe()->init(hwCtrl, chip["tx"], chip["rx"]);
                    FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(bookie.getLastFe());
                    std::ifstream cfgFile(chipConfigPath);
                    if (cfgFile) {
                        // Load config
                        shlog->info("Loading config file: {}", chipConfigPath);
                        json cfg;
                        try {
                            cfg = ScanHelper::openJsonFile(chipConfigPath);
                        } catch (std::runtime_error &e) {
                            shlog->error("Error opening chip config: {}", e.what());
                            throw(std::runtime_error("loadChips failure"));
                        }
                        feCfg->loadConfig(cfg);
                        if (chip.contains("locked"))
                            feCfg->setLocked((int)chip["locked"]);
                        cfgFile.close();
                    } else {
                        shlog->warn("Config file not found, using default!");
                        // Rename in case of multiple default configs
                        feCfg->setName(feCfg->getName() + "_" + std::to_string((int)chip["rx"]));
                        shlog->warn("Creating new config of FE {} at {}", feCfg->getName(),chipConfigPath);
                        json jTmp;
                        feCfg->writeConfig(jTmp);
                        std::ofstream oFTmp(chipConfigPath);
                        oFTmp << std::setw(4) << jTmp;
                        oFTmp.close();
                    }
                    // Save path to config
                    std::size_t botDirPos = chipConfigPath.find_last_of("/");
                    feCfgMap[bookie.getLastFe()] = chipConfigPath;
                    feCfg->setConfigFile(chipConfigPath.substr(botDirPos, chipConfigPath.length()));

                    // Create backup of current config
                    // TODO fix folder
                    std::ofstream backupCfgFile(outputDir + feCfg->getConfigFile() + ".before");
                    json backupCfg;
                    feCfg->writeConfig(backupCfg);
                    backupCfgFile << std::setw(4) << backupCfg;
                    backupCfgFile.close();
                }
            }
        }
        return chipType;        
    }

void buildHistogrammers( std::map<FrontEnd*,
                         std::unique_ptr<DataProcessor>>& histogrammers,
                         const std::string& scanType,
                         std::vector<FrontEnd*>& feList,
                         ScanBase* s, std::string outputDir) {
    bhlog->info("Loading histogrammer ...");
    json scanCfg;
    try {
        scanCfg = ScanHelper::openJsonFile(scanType);
    } catch (std::runtime_error &e) {
        bhlog->error("Opening scan config: {}", e.what());
        throw("buildHistogrammer failure");
    }
    json histoCfg = scanCfg["scan"]["histogrammer"];
    json anaCfg = scanCfg["scan"]["analysis"];

    for (FrontEnd *fe : feList ) {
        if (fe->isActive()) {
            // TODO this loads only FE-i4 specific stuff, bad
            // Load histogrammer
            histogrammers[fe].reset( new HistogrammerProcessor );
            auto& histogrammer = static_cast<HistogrammerProcessor&>( *(histogrammers[fe]) );

            histogrammer.connect(fe->clipData, fe->clipHisto);

            auto add_histo = [&](std::string algo_name) {
                auto histo = StdDict::getHistogrammer(algo_name);
                if(algo_name == "DataArchiver") {
                    auto archiver = dynamic_cast<DataArchiver*>(histo.get());
                    std::string output_filename = (outputDir + dynamic_cast<FrontEndCfg*>(fe)->getName() + "_data.raw");
                    bool status = archiver->open(output_filename);
                    if(!status) {
                        bhlog->error("Unable to open DataArchiver output file \"{}\"", output_filename);
                        throw std::runtime_error("Can't open requested output data file \"" + output_filename + "\"");
                    } 
                }
                if(histo) {
                    bhlog->debug(" ... adding {}", algo_name);
                    histogrammer.addHistogrammer(std::move(histo));
                } else {
                    bhlog->error("Error, Histogrammer \"{}\" unknown, skipping!", algo_name);
                }
            };

            try {
                int nHistos = histoCfg["n_count"];

                for (int j=0; j<nHistos; j++) {
                    std::string algo_name = histoCfg[std::to_string(j)]["algorithm"];
                    add_histo(algo_name);
                }
            } catch(/* json::type_error &te*/ ... ) { //FIXME
                int nHistos = histoCfg.size();
                for (int j=0; j<nHistos; j++) {
                    std::string algo_name = histoCfg[j]["algorithm"];
                    add_histo(algo_name);
                }
            }
            histogrammer.setMapSize(fe->geo.nCol, fe->geo.nRow);
        }
    }
    bhlog->info("... done!");
}

// A 2D vector of int to store algorithm indices for all tiers of analyses
using AlgoTieredIndex = std::vector<std::vector<int>>;

void buildAnalyses( std::map<FrontEnd*,
                    std::vector<std::unique_ptr<DataProcessor>> >& analyses,
                    const std::string& scanType, Bookkeeper& bookie,
                    ScanBase* s, FeedbackClipboardMap *fbData, int mask_opt) {
    if (scanType.find("json") != std::string::npos) {
        balog->info("Loading analyses ...");
        json scanCfg;
        try {
            scanCfg = ScanHelper::openJsonFile(scanType);
        } catch (std::runtime_error &e) {
            balog->error("Opening scan config: {}", e.what());
            throw("buildAnalyses failure");
        }
        json anaCfg = scanCfg["scan"]["analysis"];

        // Parse scan config and build analysis hierarchy
        // Use a 2D vector to hold algorithm indices for all tiers of analysis processors
        AlgoTieredIndex algoIndexTiers;
        try {
            buildAnalysisHierarchy(algoIndexTiers, anaCfg);
        } catch (std::runtime_error &e) {
            balog->error("Building analysis hierarchy: {}", e.what());
            throw("buildAnalyses failure");
        }

        for (FrontEnd *fe : bookie.feList ) {
            if (fe->isActive()) {
                // TODO this loads only FE-i4 specific stuff, bad
                // TODO hardcoded
                auto channel = dynamic_cast<FrontEndCfg*>(fe)->getRxChannel();

                for (unsigned t=0; t<algoIndexTiers.size(); t++) {
                    // Before adding new analyses
                    bool hasUpstreamAnalyses = false;
                    if (t > 0) { // ie. not analyses[fe].empty()
                        auto& ana_prev = static_cast<AnalysisProcessor&>( *(analyses[fe].back()) );
                        hasUpstreamAnalyses = not ana_prev.empty();
                    }

                    // Add analysis processors
                    analyses[fe].emplace_back( new AnalysisProcessor(&bookie, channel) );
                    auto& ana = static_cast<AnalysisProcessor&>( *(analyses[fe].back()) );

                    // Create the ClipBoard to store its output and establish connection
                    fe->clipResult->emplace_back(new ClipBoard<HistogramBase>());
                    if (t==0) {
                        ana.connect(s, fe->clipHisto, (fe->clipResult->back()).get(), &((*fbData)[channel]) );
                    } else {
                        ana.connect(s, (*(fe->clipResult->rbegin()+1)).get(),
                                    (*(fe->clipResult->rbegin())).get(),
                                    &((*fbData)[channel]), true);
                    }

                    auto add_analysis = [&](std::string algo_name, json& j) {
                        auto analysis = StdDict::getAnalysis(algo_name);
                        if(analysis) {
                            balog->debug("  ... adding {}", algo_name);
                            analysis->loadConfig(j);
                            // If it requires dependency
                            if (analysis->requireDependency() and not hasUpstreamAnalyses) {
                                balog->error("Analysis {} requires outputs from other analyses", algo_name);
                                throw("buildAnalyses failure");
                            }

                            balog->debug(" connecting feedback (if required)");
                            // analysis->connectFeedback(&(*fbData)[channel]);
                            ana.addAlgorithm(std::move(analysis));
                        } else {
                            balog->error("Error, Analysis Algorithm \"{} unknown, skipping!", algo_name);
                        }
                    };

                    // Add all AnalysisAlgorithms of the t-th tier
                    for (int aIndex : algoIndexTiers[t]) {
                        std::string algo_name = anaCfg[std::to_string(aIndex)]["algorithm"];
                        json algo_config = anaCfg[std::to_string(aIndex)]["config"];
                        add_analysis(algo_name, algo_config);
                    }

                    // Disable masking of pixels
                    if(mask_opt == 0) {
                        balog->info("Disabling masking for this scan!");
                        ana.setMasking(false);
                    }
                    ana.setMapSize(fe->geo.nCol, fe->geo.nRow);
                } // for (unsigned t=0; t<algoIndexTiers.size(); t++)
            } // if (fe->isActive())
        } // for (FrontEnd *fe : bookie.feList )
    }
}

void buildAnalysisHierarchy(AlgoTieredIndex &indexTiers, const json &anaCfg) {
    if (!anaCfg.contains("n_count"))
        throw std::runtime_error("No \"n_count\" field in analysis config");

    int nAnas = anaCfg["n_count"];
    balog->debug("Found {} analysis!", nAnas);

    std::map<std::string, int> tierMap; // key: algorithm name; value: tier
    // Pre-fill the map with all algorithms in the configuration
    for (unsigned ialgo = 0; ialgo < nAnas; ++ialgo) {
        tierMap[ anaCfg[std::to_string(ialgo)]["algorithm"] ] = -1;
    }

    auto fillIndexVector = [&indexTiers](unsigned tier, unsigned index) {
        while (indexTiers.size() <= tier) {
            indexTiers.emplace_back();
        }
        indexTiers[tier].push_back(index);
    };

    // Algorithm indices
    std::deque<unsigned> indices(nAnas);
    std::iota(std::begin(indices), std::end(indices), 0);
    int loopcnt = 0;

    while (not indices.empty()) {
        int j = indices.front();
        indices.pop_front();

        std::string algo_name = anaCfg[std::to_string(j)]["algorithm"];
        if (!anaCfg[std::to_string(j)].contains("dependOn")) {
            // This algorithm does not depend on the results of others
            // It can be placed at the first tier
            tierMap[algo_name] = 0;
            fillIndexVector(0, j);
        } else {
            // This algorithm depends on outputs of other algorithms
            int maxuptier = 0;
            // Check all algorithms on which this one depends
            for (unsigned k=0; k<anaCfg[std::to_string(j)]["dependOn"].size(); k++) {
                std::string upstream = anaCfg[std::to_string(j)]["dependOn"][k];

                // First check if the upstream algorithm is in the configuration
                if ( tierMap.find(upstream) == tierMap.end() ) {
                    // Algorithm is not defined in the configuration
                    balog->error("Fail to build analysis hierarchy due to unknown algorithm: {}", upstream);
                    throw std::runtime_error("buildAnalysisHierarchy failure");
                }

                if (tierMap[upstream] >= 0) {
                    // Get the tier of the upstream algorithm from tierMap
                    // and compare to the current max tier
                    if (tierMap[upstream] > maxuptier)
                        maxuptier = tierMap[upstream];
                } else {
                    // The tier of this upstream algorithm has not been determined yet.
                    // Skip for now and come back later
                    indices.push_back(j);
                    maxuptier = -1;
                    break;
                }
            }

            if (maxuptier >= 0) {
                // The tiers of all upstream algorithms on which this algorithm depends have been determined
                // So this algorithm's tier is the maximum of all upstream tiers + 1
                tierMap[algo_name] = maxuptier + 1;
                fillIndexVector(maxuptier + 1, j);
            }
        }

        loopcnt++;

        // In case it took too many loop iterations to figure out the tiers
        if (loopcnt > ((nAnas+1)*nAnas/2) ) {
            balog->error("Fail to build analysis hierarchy. This is likely due to circular dependency of analysis algorithms in the scan configuration.");
            throw std::runtime_error("buildAnalysisHierarchy failure");
        }
    } // while (not indices.empty())
}

std::string toString(int value,int digitsCount)
{
    std::ostringstream os;
    os<<std::setfill('0')<<std::setw(digitsCount)<<value;
    return os.str();
}


std::string getHostname() {
  std::string hostname = "default_host";
  if (getenv("HOSTNAME")) {
    hostname = getenv("HOSTNAME");
  } else {
    spdlog::error("HOSTNAME environmental variable not found ... using default: {}", hostname);
  }
  return hostname;
}

std::string defaultDbDirPath() {
  std::string home;
  if(getenv("HOME")) {
    home = getenv("HOME");
  } else {
    home = ".";
    spdlog::error("HOME not set, using local directory for configuration");
  }
  return home+"/.yarr/localdb";
}

std::string defaultDbCfgPath() {
  return defaultDbDirPath()+"/"+getHostname()+"_database.json";
}

std::string defaultDbSiteCfgPath() {
  return defaultDbDirPath()+"/"+getHostname()+"_site.json";
}

std::string defaultDbUserCfgPath() {
  return defaultDbDirPath()+"/user.json";
}

std::unique_ptr<ScanBase> buildScan( const std::string& scanType, Bookkeeper& bookie,  FeedbackClipboardMap *fbData) {

    shlog->info("Found Scan config, constructing scan ...");
    std::unique_ptr<ScanFactory> s ( new ScanFactory(&bookie, fbData) );
    json scanCfg;
    try {
        scanCfg = ScanHelper::openJsonFile(scanType);
    } catch (std::runtime_error &e) {
        shlog->error("Opening scan config: {}", e.what());
        throw("buildScan failure");
    }

    s->loadConfig(scanCfg);

    return s;
}

std::string  createOutputDir(const std::string &scanType, int runCounter, std::string &outputDir) {
    // Generate output directory path
    std::size_t pathPos = scanType.find_last_of('/');
    std::size_t suffixPos = scanType.find_last_of('.');
    std::string strippedScan;
    if (pathPos != std::string::npos && suffixPos != std::string::npos) {
        strippedScan = scanType.substr(pathPos + 1, suffixPos - pathPos - 1);
    } else {
        strippedScan = scanType;
    }

    outputDir += (ScanHelper::toString(runCounter, 6) + "_" + strippedScan + "/");
    std::string cmdStr = "mkdir -p "; //I am not proud of this ):
    cmdStr += outputDir;
    int sysExSt = system(cmdStr.c_str());
    if (sysExSt != 0) {
        shlog->error("Error creating output directory - plots might not be saved!");
    }
    return strippedScan;
}

void createSymlink(const std::string &dataDir,const std::string &strippedScan, int runCounter) {
    std::string cmdStr = "rm -f " + dataDir + "last_scan && ln -s " + ScanHelper::toString(runCounter, 6) + "_" + strippedScan + " " + dataDir + "last_scan";
    int sysExSt = system(cmdStr.c_str());
    if(sysExSt != 0){
        shlog->error("Error creating symlink to output directory!");
    }
}
std::string timestamp(std::time_t now) {
    struct tm *lt = std::localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%F_%H:%M:%S", lt);
    return timestamp;
}
void banner(std::shared_ptr<spdlog::logger> &logger, const std::string &msg) {
    unsigned len=msg.length()+8;
    std::string frame = "\033[1;31m" + std::string(len,'#') + "\033[0m";
    std::string body  = "\033[1;31m##  " + msg + "  ##\033[0m";
    logger->info(frame);
    logger->info(body);
    logger->info(frame);
}

void listChips() {
    for(std::string &chip_type: StdDict::listFrontEnds()) {
        std::cout << "  " << chip_type << "\n";
    }
}

void listProcessors() {
    for(std::string &proc_type: StdDict::listDataProcessors()) {
        std::cout << "  " << proc_type << "\n";
    }
}

void listScans() {
    for(std::string &scan_name: StdDict::listScans()) {
        std::cout << "  " << scan_name << "\n";
    }
}

void listControllers() {
    for(auto &h: StdDict::listHwControllers()) {
        std::cout << "  " << h << std::endl;
    }
}

void listScanLoopActions() {
    for(auto &la: StdDict::listLoopActions()) {
        std::cout << "  " << la << std::endl;
    }
}

void listKnown() {
    std::cout << " Known HW controllers:\n";
    listControllers();

    std::cout << " Known Chips:\n";
    listChips();

    std::cout << " Known Processors:\n";
    listProcessors();

    std::cout << " Known Scans:\n";
    listScans();

    std::cout << " Known ScanLoop actions:\n";
    listScanLoopActions();

    std::cout << " Known loggers:\n";
    logging::listLoggers();
}

bool lsdir(const std::string &dataDir) {
     std::string lsCmd = "ls -1 " + dataDir;
     int result = system(lsCmd.c_str());
     return result>=0;
}

void printHelp() {
    std::string dbCfgPath = defaultDbCfgPath();
    std::string dbSiteCfgPath = defaultDbSiteCfgPath();
    std::string dbUserCfgPath = defaultDbDirPath();

    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -n <threads> : Set number of processing threads." << std::endl;
    std::cout << " -s <scan_type> : Scan config" << std::endl;
    std::cout << " -c <connectivity.json> [<cfg2.json> ...]: Provide connectivity configuration, can take multiple arguments." << std::endl;
    std::cout << " -r <ctrl.json> Provide controller configuration." << std::endl;
    std::cout << " -t <target_charge> [<tot_target>] : Set target values for threshold/charge (and tot)." << std::endl;
    std::cout << " -p: Enable plotting of results." << std::endl;
    std::cout << " -o <dir> : Output directory. (Default ./data/)" << std::endl;
    std::cout << " -m <int> : 0 = pixel masking disabled, 1 = start with fresh pixel mask, default = pixel masking enabled" << std::endl;
    std::cout << " -k: Report known items (Scans, Hardware etc.)\n";
    std::cout << " -W: Enable using Local DB." << std::endl;
    std::cout << " -d <database.json> : Provide database configuration. (Default " << dbCfgPath << ")" << std::endl;
    std::cout << " -i <site.json> : Provide site configuration. (Default " << dbSiteCfgPath << ")" << std::endl;
    std::cout << " -u <user.json> : Provide user configuration. (Default " << dbUserCfgPath << ")" << std::endl;
    std::cout << " -l <log_cfg.json> : Provide logger configuration." << std::endl;
    std::cout << " -Q: Set QC scan mode." << std::endl;
    std::cout << " -I: Set interactive mode." << std::endl;
}
int parseOptions(int argc, char *argv[], ScanOpts &scanOpts) {
    scanOpts.dbCfgPath = defaultDbCfgPath();
    scanOpts.dbSiteCfgPath = defaultDbSiteCfgPath();
    scanOpts.dbUserCfgPath = defaultDbUserCfgPath();
    for (int i=1;i<argc;i++)scanOpts. commandLineStr.append(std::string(argv[i]).append(" "));
    scanOpts.progName=argv[0];
    int c;
    while ((c = getopt(argc, argv, "hn:ks:n:m:g:r:c:t:po:Wd:u:i:l:QI")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'n':
                scanOpts.nThreads = atoi(optarg);
                break;
                break;
            case 'k':
                ScanHelper::listKnown();
                return 0;
            case 's':
                scanOpts.scan_config_provided = true;
                scanOpts.scanType = std::string(optarg);
                break;
            case 'm':
                scanOpts.mask_opt = atoi(optarg);
                break;
            case 'c':
                optind -= 1; //this is a bit hacky, but getopt doesn't support multiple
                //values for one option, so it can't be helped
                for (; optind < argc && *argv[optind] != '-'; optind += 1) {
                    scanOpts.cConfigPaths.push_back(std::string(argv[optind]));
                }
                break;
            case 'r':
                scanOpts.ctrlCfgPath = std::string(optarg);
                break;
            case 'p':
                scanOpts.doPlots = true;
                break;
            case 'o':
                scanOpts.outputDir = std::string(optarg);
                if (scanOpts.outputDir.back() != '/')
                    scanOpts.outputDir = scanOpts.outputDir + "/";
                break;
            case 't':
                optind -= 1; //this is a bit hacky, but getopt doesn't support multiple
                //values for one option, so it can't be helped
                for (; optind < argc && *argv[optind] != '-'; optind += 1) {
                    switch (count) {
                        case 0:
                            scanOpts.target_charge = atoi(argv[optind]);
                            break;
                        case 1:
                            scanOpts.target_tot = atoi(argv[optind]);
                            break;
                        default:
                            spdlog::error("Can only receive max. 2 parameters with -t!!");
                            break;
                    }
                    count++;
                }
                break;
            case 'W': // Write to DB
                scanOpts.dbUse = true;
                break;
            case 'd': // Database config file
                scanOpts.dbCfgPath = std::string(optarg);
                break;
            case 'l': // Logger config file
                scanOpts.logCfgPath = std::string(optarg);
                break;
            case 'i': // Database config file
                scanOpts.dbSiteCfgPath = std::string(optarg);
                break;
            case 'u': // Database config file
                scanOpts.dbUserCfgPath = std::string(optarg);
                break;
            case 'Q':
                scanOpts.setQCMode = true;
                break;
            case 'I':
                scanOpts.setInteractiveMode = true;
                break;
            case '?':
                if (optopt == 's' || optopt == 'n') {
                    spdlog::error("Option {} requires a parameter! (Proceeding with default)", (char) optopt);
                } else if (optopt == 'g' || optopt == 'c') {
                    spdlog::error("Option {} requires a parameter! Aborting... ", (char) optopt);
                    return -1;
                } else {
                    spdlog::error("Unknown parameter: {}", (char) optopt);
                }
                break;
            default:
                spdlog::critical("Error while parsing command line parameters!");
                return -1;
        }
    }
    return 1;
}
} // Close namespace}
