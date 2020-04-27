#include "ScanHelper.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <iomanip>

#include "AllAnalyses.h"
#include "AllChips.h"
#include "AllHistogrammers.h"
#include "AllHwControllers.h"

#include "AnalysisAlgorithm.h"
#include "HistogramAlgorithm.h"

// Need to pass info to DataArchiver constructor
#include "Fei4Histogrammer.h"

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
        if (config["chipType"].empty() || config["chips"].empty()) {
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
                        feCfg->fromFileJson(cfg);
                        if (!chip["locked"].empty())
                            feCfg->setLocked((int)chip["locked"]);
                        cfgFile.close();
                    } else {
                        shlog->warn("Config file not found, using default!");
                        // Rename in case of multiple default configs
                        feCfg->setName(feCfg->getName() + "_" + std::to_string((int)chip["rx"]));
                        shlog->warn("Creating new config of FE {} at {}", feCfg->getName(),chipConfigPath);
                        json jTmp;
                        feCfg->toFileJson(jTmp);
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
                    feCfg->toFileJson(backupCfg);
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
                if(histo) {
                    bhlog->debug("  ... adding {}", algo_name);
                    histogrammer.addHistogrammer(std::move(histo));
                } else if (algo_name == "DataArchiver") {
                    histo.reset(new DataArchiver((outputDir + dynamic_cast<FrontEndCfg*>(fe)->getName() + "_data.raw")));
                    histogrammer.addHistogrammer(std::move(histo));
                    bhlog->debug("  ... adding {}", algo_name);
                } else {
                    bhlog->error("Error, Histogrammer \"{} unknown, skipping!", algo_name);
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

void buildAnalyses( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& analyses, const std::string& scanType, Bookkeeper& bookie, ScanBase* s, int mask_opt) {
    if (scanType.find("json") != std::string::npos) {
        balog->info("Loading analyses ...");
        json scanCfg;
        try {
            scanCfg = ScanHelper::openJsonFile(scanType);
        } catch (std::runtime_error &e) {
            balog->error("Opening scan config: {}", e.what());
            throw("buildAnalyses failure");
        }
        json histoCfg = scanCfg["scan"]["histogrammer"];
        json anaCfg = scanCfg["scan"]["analysis"];

        for (FrontEnd *fe : bookie.feList ) {
            if (fe->isActive()) {
                // TODO this loads only FE-i4 specific stuff, bad
                // TODO hardcoded
                analyses[fe].reset( new AnalysisProcessor(&bookie, dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()) );
                auto& ana = static_cast<AnalysisProcessor&>( *(analyses[fe]) );
                ana.connect(s, fe->clipHisto, fe->clipResult);

                auto add_analysis = [&](std::string algo_name) {
                    auto analysis = StdDict::getAnalysis(algo_name);
                    if(analysis) {
                        balog->debug("  ... adding {}", algo_name);
                        ana.addAlgorithm(std::move(analysis));
                    } else {
                        balog->error("Error, Analysis Algorithm \"{} unknown, skipping!", algo_name);
                    }
                };

                try {
                  int nAnas = anaCfg["n_count"];
                  balog->debug("Found {} Analysis!", nAnas);
                  for (int j=0; j<nAnas; j++) {
                    std::string algo_name = anaCfg[std::to_string(j)]["algorithm"];
                    add_analysis(algo_name);
                  }
                  ana.loadConfig(anaCfg);
                } catch(/* json::type_error &te */ ...) { //FIXME
                  int nAnas = anaCfg.size();
                  balog->debug("Found {} Analysis!", nAnas);
                  for (int j=0; j<nAnas; j++) {
                    std::string algo_name = anaCfg[j]["algorithm"];
                    add_analysis(algo_name);
                  }
                }

                // Disable masking of pixels
                if(mask_opt == 0) {
                    balog->info("Disabling masking for this scan!");
                    ana.setMasking(false);
                }
                ana.setMapSize(fe->geo.nCol, fe->geo.nRow);
            }
        }
    }
}

} // Close namespace}
