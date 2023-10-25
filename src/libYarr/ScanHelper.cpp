#include "ScanHelper.h"

#include <iostream>
#include <exception>
#include <iomanip>
#include <memory>
#include <numeric>
#include <getopt.h>
#include <filesystem>
namespace fs = std::filesystem;

#include "AllAnalyses.h"
#include "AllChips.h"
#include "AllHistogrammers.h"
#include "AllHwControllers.h"
#include "AllProcessors.h"
#include "AllStdActions.h"

#include "AnalysisAlgorithm.h"
#include "HistogramAlgorithm.h"
#include "StdHistogrammer.h" // needed for special handling of DataArchiver
#include "StdAnalysis.h" // needed for special handling of HistogramArchiver
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
    json openJsonFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file) {
            throw std::runtime_error("could not open file");
        }
        json j;
        try {
            j = json::parse(file);
        } catch (json::parse_error &e) {
            throw std::runtime_error(e.what());
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
        if (!ctrlCfg.contains({"ctrlCfg", "type"})) {
            shlog->critical("Controller type not specified!");
            throw (std::runtime_error("loadController failure"));
        }

        std::string controller = ctrlCfg["ctrlCfg"]["type"];

        hwCtrl = StdDict::getHwController(controller);

        if(hwCtrl) {
            shlog->info("Found controller of type: {}", controller);
            if (!ctrlCfg.contains({"ctrlCfg", "cfg"})) {
                shlog->error("Could not find cfg for controller, skipping!");
            } else {
                hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
            }
            shlog->info("Loaded controller config:");
            json cfg = ctrlCfg["ctrlCfg"]["cfg"];
            if(cfg.contains("__feCfg_data__")) cfg.erase("__feCfg_data__");
            std::stringstream ss;
            ss << cfg;
            std::string line;
            while (std::getline(ss, line)) shlog->info("~~~ {}", line);

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

    std::string loadChipConfigs(json &config, bool createConfig) {
        return loadChipConfigs(config, createConfig, "");
    }

    std::string loadChipConfigs(json &config, const bool &createConfig, const std::string &dir) {
        std::string chipType;
        if (!config.contains("chipType") || !config.contains("chips")) {
            shlog->error("Invalid config, chip type or chips not specified!");
            throw (std::runtime_error("buildChips failure"));
        }
        chipType = config["chipType"];
        shlog->info("Chip type: {}", chipType);
        shlog->info("Chip count {}", config["chips"].size());
        // Loop over chips
        for (unsigned i = 0; i < config["chips"].size(); i++) {
            shlog->info("Loading chip #{}", i);
            json &chip = config["chips"][i];
            if (!chip.contains("enable") || !chip.contains("config")) {
                shlog->error("Connectivity config for chip #{} malformed, skipping!", i);
                continue;
            }

            if (chip["enable"] == 0) {
                shlog->warn(" ... chip not enabled, skipping!");
                continue;
            }
            // Check if config path is:
            // - relative to exectuteable (default)
            // - relative to connectivity file
            // - absolute
            // - relative to "YARR_CONFIG_PATH" env var
            // - database (TODO)
            std::string chipConfigPath;
            bool pullFromDb = false;
            if (chip.contains("path")) {
                if (chip["path"] == "relToExec") {
                    chipConfigPath = chip["config"];
                } else if (chip["path"] == "relToCon") {
                    chipConfigPath = dir + "/" + std::string(chip["config"]);
                } else if (chip["path"] == "abs") {
                    chipConfigPath = chip["config"];
                } else if (chip["path"] == "relToYarrPath") {
                    std::string yarr_path = "";
                    if (std::getenv("YARR_CONFIG_PATH"))
                        yarr_path = std::string(std::getenv("YARR_CONFIG_PATH"));
                    chipConfigPath = yarr_path + "/" + std::string(chip["config"]);
                } else if (chip["path"] == "db") {
                    pullFromDb = true;
                }
            } else {
                // default is relative to exec
                chipConfigPath = chip["config"];
            }
            chip["__config_path__"] = chipConfigPath;

            // TODO should be a shared pointer
            auto fe=StdDict::getFrontEnd(chipType);
            auto *feCfg = dynamic_cast<FrontEndCfg *>(fe.get());
            if (std::filesystem::exists(chipConfigPath)) {
                // Load config
                shlog->info("Loading config file: {}", chipConfigPath);
                json cfg;
                try {
                    cfg = ScanHelper::openJsonFile(chipConfigPath);
                } catch (std::runtime_error &e) {
                    shlog->error("Error opening chip config: {}", e.what());
                    throw (std::runtime_error("buildChips failure"));
                }
                chip["__config_data__"] = cfg;
            } else {
                shlog->warn("Config file not found, creating new file from defaults!");
                // Rename in case of multiple default configs
                feCfg->setName(feCfg->getName() + "_" + std::to_string((int) chip["rx"]));
                shlog->warn("Creating new config of FE {} at {}", feCfg->getName(), chipConfigPath);
                json jTmp;
                feCfg->writeConfig(jTmp);
                chip["__config_data__"] = jTmp;
                if(createConfig) {
                    if(chip["enable"] == 1) {
                        std::ofstream oFTmp(chipConfigPath);
                        oFTmp << std::setw(4) << jTmp;
                        oFTmp.close();
                    }
                }
            }
        }
        return chipType;
    }

    void buildRawDataProcs( std::map<unsigned, std::unique_ptr<FeDataProcessor> > &procs,
            Bookkeeper &bookie,
            const std::string &chipType) {
        bhlog->info("Loading RawData processors ..");
        for (unsigned id = 0; id<bookie.getNumOfEntries(); id++) {
            FrontEnd *fe = bookie.getEntry(id).fe;
            procs[id] = StdDict::getDataProcessor(chipType);
            procs[id]->connect(dynamic_cast<FrontEndCfg*>(fe), &bookie.getEntry(id).fe->clipRawData, &bookie.getEntry(id).fe->clipData);
            procs[id]->connect(&bookie.getEntry(id).fe->clipProcFeedback);
            // TODO load global processor config
            // TODO load chip specific config
        }
    }

    std::string buildChips(const json &config, Bookkeeper &bookie, HwController *hwCtrl,
            std::map<unsigned, std::array<std::string,2>> &feCfgMap) {
        const std::string &chipType = config["chipType"];
        shlog->info("Chip type: {}", chipType);
        shlog->info("Chip count {}", config["chips"].size());
        // Loop over chips
        for (unsigned i=0; i<config["chips"].size(); i++) {
            shlog->info("Loading chip config #{}", i);
            const json &chip = config["chips"][i];
            if (chip["enable"] == 0) {
                shlog->warn(" ... chip not enabled, skip config!");
                continue;
            }
            std::string chipConfigPath = chip["__config_path__"];
            bookie.addFe(StdDict::getFrontEnd(chipType).release(), chip["tx"], chip["rx"]);
            bookie.getLastFe()->init(hwCtrl, chip["tx"], chip["rx"]);
            bookie.getLastFe()->init(hwCtrl, chip["tx"], chip["rx"]);
            auto *feCfg = dynamic_cast<FrontEndCfg*>(bookie.getLastFe());
            const json &cfg=chip["__config_data__"];
            feCfg->loadConfig(cfg);
            if (chip.contains("locked"))
                feCfg->setLocked((int)chip["locked"]);
            std::size_t botDirPos = chipConfigPath.find_last_of('/');
            std::string  cfgFile=chipConfigPath.substr(botDirPos, chipConfigPath.length());
            feCfgMap[bookie.getId(bookie.getLastFe())] = {chipConfigPath, cfgFile};
        }
        return chipType;
    }


    int loadConfigFile(const ScanOpts &scanOpts, bool writeConfig, json &config) {
        // load controller configs
        json ctrlCfg;
        try {
            ctrlCfg = ScanHelper::openJsonFile(scanOpts.ctrlCfgPath);
        } catch(std::runtime_error &e) {
            shlog->error("Error opening controller config ({}): {}",
                    scanOpts.ctrlCfgPath, e.what());
            throw (std::runtime_error("loadConfigFile failure"));
        }

        if(!ctrlCfg.contains({"ctrlCfg", "cfg"})) {
            shlog->critical("#ERROR# invalid controller config");
            return -1;
        }
        json &cfg=ctrlCfg["ctrlCfg"]["cfg"];

        // Emulator specific case
        if(cfg.contains("feCfg")) {
            try {
                cfg["__feCfg_data__"]=ScanHelper::openJsonFile(cfg["feCfg"]);
            } catch (std::runtime_error &e) {
                shlog->critical("#ERROR# opening emulator FE model config: {}", e.what());
                return -1;
            }
        }

        // Emulator specific case
        if(cfg.contains("chipCfg")) {
            try {
                cfg["__chipCfg_data__"]=ScanHelper::openJsonFile(cfg["chipCfg"]);
            } catch (std::runtime_error &e) {
                shlog->critical("#ERROR# opening emulator chip model config ({}): {}", (std::string)cfg["chipCfg"], e.what());
                return -1;
            }
        }

        // load FE configs
        json chipConfig=json::array();
        for (std::string const &sTmp: scanOpts.cConfigPaths) {
            json feconfig;
            try {
                feconfig = ScanHelper::openJsonFile(sTmp);
            } catch (std::runtime_error &e) {
                shlog->critical("#ERROR# opening connectivity or chip configs ({}): {}", sTmp, e.what());
                return -1;
            }
            loadChipConfigs(feconfig, writeConfig, Utils::dirFromPath(sTmp));
            chipConfig.push_back(feconfig);
        }

        // Load scans
        json scan;
        try {
            if (!scanOpts.scanType.empty())
                scan = openJsonFile(scanOpts.scanType);
        } catch (std::runtime_error &e) {
            shlog->critical("#ERROR# opening scan config: {}", e.what());
            return -1;
        }

        config["scanCfg"]=scan;
        config["chipConfig"]=chipConfig;
        config["ctrlConfig"]=ctrlCfg;
        return 0;
    }

    void buildHistogrammers( std::map<unsigned, std::unique_ptr<HistoDataProcessor>>& histogrammers,
                             const json& scanCfg,
                             Bookkeeper &bookie,
                             std::string outputDir) {
        bhlog->info("Loading histogrammer ...");

        const json &histoCfg = scanCfg["scan"]["histogrammer"];
        const json &anaCfg = scanCfg["scan"]["analysis"];

        for (unsigned id=0; id<bookie.getNumOfEntries(); id++) {
            FrontEnd *fe = bookie.getEntry(id).fe;
            if (fe->isActive()) {
                // Load histogrammer
                histogrammers[id] = std::make_unique<HistogrammerProcessor>( );
                auto& histogrammer = dynamic_cast<HistogrammerProcessor&>( *(histogrammers[id]) );

                histogrammer.connect(&fe->clipData, &fe->clipHisto);

                auto add_histo = [&](const std::string& algo_name) {
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

                if(histoCfg.contains("n_count")) {
                    int nHistos = histoCfg["n_count"];

                    for (int j=0; j<nHistos; j++) {
                        std::string algo_name = histoCfg[std::to_string(j)]["algorithm"];
                        add_histo(algo_name);
                    }
                } else {
                    std::size_t nHistos = histoCfg.size();
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

    void buildAnalyses( std::map<unsigned,
            std::vector<std::unique_ptr<AnalysisDataProcessor>> >& analyses,
            const json& scanCfg, Bookkeeper& bookie,
            ScanBase* s, FeedbackClipboardMap *fbData, int mask_opt, std::string outputDir) {
        balog->info("Loading analyses ...");

        const json &anaCfg = scanCfg["scan"]["analysis"];

        // Parse scan config and build analysis hierarchy
        // Use a 2D vector to hold algorithm indices for all tiers of analysis processors
        AlgoTieredIndex algoIndexTiers;
        try {
            buildAnalysisHierarchy(algoIndexTiers, anaCfg);
        } catch (std::runtime_error &e) {
            balog->error("Building analysis hierarchy: {}", e.what());
            throw(std::runtime_error("buildAnalyses failure"));
        }

        bool indexed;

        // Is this an array of objects, or "n_count" + indexed by string "0"
        if (anaCfg.contains("n_count")) {
            indexed = true;
        } else {
            indexed = false;
        }

        auto get_algorithm = [indexed, &anaCfg](int index) {
            if(indexed) {
                return anaCfg[std::to_string(index)];
            } else {
                return anaCfg[index];
            }
        };

        for (unsigned id=0; id<bookie.getNumOfEntries(); id++ ) {
            FrontEnd *fe = bookie.getEntry(id).fe;
            if (fe->isActive()) {
                for (unsigned t=0; t<algoIndexTiers.size(); t++) {
                    // Before adding new analyses
                    bool hasUpstreamAnalyses = false;
                    if (t > 0) { // ie. not analyses[fe].empty()
                        auto& ana_prev = dynamic_cast<AnalysisProcessor&>( *(analyses[id].back()) );
                        hasUpstreamAnalyses = not ana_prev.empty();
                    }

                    // Add analysis processors
                    analyses[id].emplace_back( new AnalysisProcessor(&bookie, id) );
                    auto& ana = dynamic_cast<AnalysisProcessor&>( *(analyses[id].back()) );

                    // Create the ClipBoard to store its output and establish connection
                    fe->clipResult.emplace_back(new ClipBoard<HistogramBase>());
                    if (t==0) {
                        ana.connect(s, &fe->clipHisto, (fe->clipResult.back()).get(), &((*fbData)[id]) );
                    } else {
                        ana.connect(s, (*(fe->clipResult.rbegin()+1)).get(),
                                (*(fe->clipResult.rbegin())).get(),
                                &((*fbData)[id]), true);
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
                            if(algo_name == "HistogramArchiver") {
                                auto archiver = dynamic_cast<HistogramArchiver*>(analysis.get());
                                archiver->setOutputDirectory(outputDir);
                            }

                            ana.addAlgorithm(std::move(analysis));
                        } else {
                            balog->error("Error, Analysis Algorithm \"{} unknown, skipping!", algo_name);
                        }
                    };


                    // Add all AnalysisAlgorithms of the t-th tier
                    for (int aIndex : algoIndexTiers[t]) {
                        std::string algo_name = get_algorithm(aIndex)["algorithm"];
                        json algo_config = get_algorithm(aIndex)["config"];
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
        } // for
    }

    void buildAnalysisHierarchy(AlgoTieredIndex &indexTiers, const json &anaCfg) {
        bool indexed;

        // Is this an array of objects, or "n_count" + indexed by string "0"
        if (anaCfg.contains("n_count")) {
            indexed = true;
        } else {
            indexed = false;
        }

        const auto &get_algorithm = [indexed, &anaCfg](int index) {
            if(indexed) {
                return anaCfg[std::to_string(index)];
            } else {
                return anaCfg[index];
            }
        };

        int nAnas = indexed ? (size_t)anaCfg["n_count"] : anaCfg.size();

        balog->debug("Found {} analysis!", nAnas);

        std::map<std::string, int> tierMap; // key: algorithm name; value: tier
        // Pre-fill the map with all algorithms in the configuration
        for (unsigned ialgo = 0; ialgo < nAnas; ++ialgo) {
            tierMap[ get_algorithm(ialgo)["algorithm"] ] = -1;
        }

        auto fillIndexVector = [&indexTiers](unsigned tier, int index) {
            while (indexTiers.size() <= tier) {
                indexTiers.emplace_back();
            }
            indexTiers[tier].push_back(index);
        };

        // Algorithm indices
        std::deque<int> indices(nAnas);
        std::iota(std::begin(indices), std::end(indices), 0);
        int loopcnt = 0;

        while (not indices.empty()) {
            int j = indices.front();
            indices.pop_front();

            const auto &algo_data = get_algorithm(j);

            std::string algo_name = algo_data["algorithm"];
            if (!algo_data.contains("dependOn")) {
                // This algorithm does not depend on the results of others
                // It can be placed at the first tier
                tierMap[algo_name] = 0;
                fillIndexVector(0, j);
            } else {
                // This algorithm depends on outputs of other algorithms
                int maxuptier = 0;
                // Check all algorithms on which this one depends
                for (unsigned k=0; k<algo_data["dependOn"].size(); k++) {
                    std::string upstream = algo_data["dependOn"][k];

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

    template<typename T>
        std::string toString(T value,int digitsCount)
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

    std::unique_ptr<ScanBase> buildScan(const json &scanCfg, Bookkeeper& bookie,  FeedbackClipboardMap *fbData) {

        shlog->info("Found Scan config, constructing scan ...");
        std::unique_ptr<ScanFactory> s ( new ScanFactory(&bookie, fbData) );
        s->loadConfig(scanCfg);

        return s;
    }

    void writeFeConfig(FrontEndCfg *feCfg, const std::string &filename) {
        std::ofstream backupCfgFile(filename);
        json backupCfg;
        feCfg->writeConfig(backupCfg);
        backupCfgFile << std::setw(4) << backupCfg;
        backupCfgFile.close();
    }

    void writeScanLog(json scanLog, const std::string &filename) {
        if (scanLog.contains({"ctrlCfg", "ctrlCfg", "cfg", "__feCfg_data__"}))
            scanLog["ctrlCfg"]["ctrlCfg"]["cfg"].erase("__feCfg_data__");
        for (std::size_t i = 0; i < scanLog["connectivity"].size(); i++) {
            json &cfg = scanLog["connectivity"][i];
            for (std::size_t j = 0; j < cfg["chips"].size(); j++) {
                if (cfg["chips"][j].contains("__config_data__"))
                    cfg["chips"][j].erase("__config_data__");
            }
        }
        // Save scan log
        std::ofstream scanLogFile(filename);
        scanLogFile << std::setw(4) << scanLog;
        scanLogFile.close();
    }

    std::string  createOutputDir(const std::string &scanType, unsigned int runCounter, std::string &outputDir) {
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

    void createSymlink(const std::string &dataDir, const std::string &strippedScan, unsigned int runCounter) {
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

    void listAnalyses() {
        for(std::string &ana_type: StdDict::listAnalyses()) {
            std::cout << "  " << ana_type << "\n";
        }
    }

    void listHistogrammers() {
        for(std::string &histo_type: StdDict::listHistogrammers()) {
            std::cout << "  " << histo_type << "\n";
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

        std::cout << " Known analysis algorithms:\n";
        listAnalyses();

        std::cout << " Known histogram algorithms:\n";
        listHistogrammers();

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
        std::cout << " -s <scan_type> : Scan config" << std::endl;
        std::cout << " -c <connectivity.json> [<cfg2.json> ...]: Provide connectivity configuration, can take multiple arguments." << std::endl;
        std::cout << " -r <ctrl.json> Provide controller configuration." << std::endl;
        std::cout << " -t <target_charge> [<tot_target>] : Set target values for threshold/charge (and tot)." << std::endl;
        std::cout << " -p: Enable plotting of results." << std::endl;
        std::cout << " -g: Enable making data pipeline graph." << std::endl;
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
        std::cout << " --skip-reset: Disable sending global front-end reset command prior to running the scan." << std::endl;
    }

    int parseOptions(int argc, char *argv[], ScanOpts &scanOpts) {
        optind = 1; // this is a global libc variable to reset getopt
        scanOpts.dbCfgPath = defaultDbCfgPath();
        scanOpts.dbSiteCfgPath = defaultDbSiteCfgPath();
        scanOpts.dbUserCfgPath = defaultDbUserCfgPath();
        for (int i=1;i<argc;i++)scanOpts.commandLineStr.append(std::string(argv[i]).append(" "));
        scanOpts.progName=argv[0];
        const struct option long_options[] =
        {
            {"skip-reset", no_argument, 0, 'z'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};
        int c;
        while (true) {
            int opt_index=0;
            c = getopt_long(argc, argv, "hn:ks:n:m:r:c:t:pgo:W:d:u:i:l:QIz", long_options, &opt_index);
            int count = 0;
            if(c == -1) break;
            switch (c) {
                case 'h':
                    printHelp();
                    return 0;
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
                case 'g':
                    scanOpts.makeGraph = true;
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
		    scanOpts.dbTag = std::string(optarg);
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
                case 'z':
                    scanOpts.doResetBeforeScan = false;
                    break;
                case '?':
                    if (optopt == 's') {
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
                    std::cout << "Rerun with --help for more information\n";
                    return -1;
            }
        }

        if(scanOpts.ctrlCfgPath.empty()) {
            spdlog::critical("Controller config required (-r)");
            std::cout << "Rerun with --help for more information\n";
            return -1;
        }

        if(scanOpts.cConfigPaths.empty()) {
            spdlog::critical("Error: no connectivity config files given, please specify config file name under -c option, even if file does not exist!");
            std::cout << "Rerun with --help for more information\n";
            return -1;
        }

        return 1;
    }
} // Close namespace
