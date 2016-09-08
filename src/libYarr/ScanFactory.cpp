// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Factory
// # Comment: Depends on dictionary for FE
// ################################

#include "ScanFactory.h"

ScanFactory::ScanFactory() {
}

void ScanFactory::init() {
}

void ScanFactory::preScan() {

}

void ScanFactory::postScan() {

}

void ScanFactory::loadConfig(json &scanCfg) {
    m_config = scanCfg;
    std::cout << "Loading Scan:" << std::endl;
    
    std::string name = scanCfg["scan"]["name"];
    std::cout << "  Name: " << name << std::endl;

    int nLoops = scanCfg["scan"]["loops"]["n_loops"];
    std::cout << "  Number of Loops: " << nLoops << std::endl;

    for (unsigned i=0; i<nLoops; i++) {
        std::cout << "  Loading Loop #" << i << std::endl;
        std::string loopAction = scanCfg["scan"]["loops"][std::to_string(i)]["loopAction"];
        std::cout << "  Type: " << loopAction << std::endl;
        std::shared_ptr<LoopActionBase> action;
        if (loopAction.find("Std") != std::string::npos) {
            action.reset(StdDict::getLoopAction(loopAction));
        } else if (loopAction.find("Fei4") != std::string::npos) {
            action.reset(Fei4Dict::getLoopAction(loopAction));
        } else {
            std::cout << "### ERROR ### => Unknown Loop Action: " << loopAction << " ... skipping!" << std::endl;
        }

        std::cout << "  Loading loop config ... " << std::endl;
        action->loadConfig(scanCfg["scan"]["loops"][std::to_string(i)]["config"]);

        std::cout << " Check config: " << std::endl;
        json tCfg;
        action->writeConfig(tCfg);
        std::cout << std::setw(4) << tCfg;
    }
                    

}
