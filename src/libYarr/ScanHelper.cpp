#include "ScanHelper.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <iomanip>

#include "logging.h"

namespace {
    auto shlog = logging::make_log("ScanHelper");
}

namespace ScanHelper {

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
                    dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->setConfigFile(chipConfigPath.substr(botDirPos, chipConfigPath.length()));

                    // Create backup of current config
                    // TODO fix folder
                    std::ofstream backupCfgFile(outputDir + dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->getConfigFile() + ".before");
                    json backupCfg;
                    dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->toFileJson(backupCfg);
                    backupCfgFile << std::setw(4) << backupCfg;
                    backupCfgFile.close();
                }
            }
        }
        return chipType;        
    }
}
