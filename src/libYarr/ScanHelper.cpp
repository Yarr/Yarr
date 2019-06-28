#include "ScanHelper.h"

#include <iostream>
#include <fstream>
#include <exception>
#include <iomanip>

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

        // Open controller config file
        std::string controller = ctrlCfg["ctrlCfg"]["type"];

        hwCtrl = StdDict::getHwController(controller);

        if(hwCtrl) {
            std::cout << "-> Found config for controller " << controller << std::endl;
            hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
        } else {
            std::cerr << "#ERROR# Unknown config type: " << ctrlCfg["ctrlCfg"]["type"] << std::endl;
            std::cout << " Known HW controllers:\n";
            for(auto &h: StdDict::listHwControllers()) {
                std::cout << "  " << h << std::endl;
            }
            std::cerr << "Aborting!" << std::endl;
            throw(std::runtime_error("loadController failure"));
        }
        return hwCtrl;
    }

    // Load connectivyt and load chips into bookkeeper
    std::string loadChips(json &config, Bookkeeper &bookie, HwController *hwCtrl, std::map<FrontEnd*, std::string> &feCfgMap, std::string &outputDir) {
        std::string chipType;
        if (config["chipType"].empty() || config["chips"].empty()) {
            std::cerr << __PRETTY_FUNCTION__ << " : invalid config, chip type or chips not specified!" << std::endl;
            throw(std::runtime_error("loadChips failure"));
        } else {
            chipType = config["chipType"];
            std::cout << "Chip Type: " << chipType << std::endl;
            std::cout << "Found " << config["chips"].size() << " chips defined!" << std::endl;
            // Loop over chips
            for (unsigned i=0; i<config["chips"].size(); i++) {
                std::cout << "Loading chip #" << i << std::endl;
                json chip = config["chips"][i];
                std::string chipConfigPath = chip["config"];
                if (chip["enable"] == 0) {
                    std::cout << " ... chip not enabled, skipping!" << std::endl;
                } else {
                    // TODO should be a shared pointer
                    bookie.addFe(StdDict::getFrontEnd(chipType).release(), chip["tx"], chip["rx"]);
                    bookie.getLastFe()->init(hwCtrl, chip["tx"], chip["rx"]);
                    FrontEndCfg *feCfg = dynamic_cast<FrontEndCfg*>(bookie.getLastFe());
                    std::ifstream cfgFile(chipConfigPath);
                    if (cfgFile) {
                        // Load config
                        std::cout << "Loading config file: " << chipConfigPath << std::endl;
                        json cfg;
                        try {
                            cfg = ScanHelper::openJsonFile(chipConfigPath);
                        } catch (std::runtime_error &e) {
                            std::cerr << "#ERROR# opening chip config: " << e.what() << std::endl;
                            throw(std::runtime_error("loadChips failure"));
                        }
                        feCfg->fromFileJson(cfg);
                        if (!chip["locked"].empty())
                            feCfg->setLocked((int)chip["locked"]);
                        cfgFile.close();
                    } else {
                        std::cout << "Config file not found, using default!" << std::endl;
                        // Rename in case of multiple default configs
                        feCfg->setName(feCfg->getName() + "_" + std::to_string((int)chip["rx"]));
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
