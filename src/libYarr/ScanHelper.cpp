#include "ScanHelper.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <iomanip>
#include <numeric>

#include "AllAnalyses.h"
#include "AllChips.h"
#include "AllHistogrammers.h"
#include "AllHwControllers.h"

#include "AnalysisAlgorithm.h"
#include "HistogramAlgorithm.h"
#include "StdHistogrammer.h" // needed for special handling of DataArchiver

#include "logging.h"

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
    std::unique_ptr<HwController> loadController(json &ctrlCfg) {
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

    // Load connectivyt and load chips into bookkeeper
    std::string loadChips(json &config, Bookkeeper &bookie, HwController *hwCtrl, std::map<FrontEnd*, std::string> &feCfgMap, std::string &outputDir) {
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

void buildHistogrammers( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& histogrammers, const std::string& scanType, std::vector<FrontEnd*>& feList, ScanBase* s, std::string outputDir) {
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

void buildAnalyses( std::map<FrontEnd*, std::vector<std::unique_ptr<DataProcessor>> >& analyses, const std::string& scanType, Bookkeeper& bookie, ScanBase* s, FeedbackClipboardMap *fbData, int mask_opt) {
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
                            // If it requires dependency
                            if (analysis->requireDependency() and not hasUpstreamAnalyses) {
                                balog->error("Analysis {} requires outputs from other analyses", algo_name);
                                throw("buildAnalyses failure");
                            }

                            balog->debug("  ... adding {}", algo_name);
                            balog->debug(" connecting feedback (if required)");
                            // analysis->connectFeedback(&(*fbData)[channel]);
                            analysis->loadConfig(j);
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

void buildAnalysisHierarchy(AlgoTieredIndex& indexTiers, json& anaCfg) {
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

} // Close namespace}
