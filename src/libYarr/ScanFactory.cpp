// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Factory
// # Comment: Depends on dictionary for FE
// ################################

#include "ScanFactory.h"

ScanFactory::ScanFactory(Bookkeeper *k) : ScanBase(k) {
}

void ScanFactory::init() {
    // TODO I don't like this, we assume the innermost loops get the data
    dynamic_cast<StdDataLoop*>((this->getLoop(this->size()-1)).get())->connect(g_data);    
    
    engine.init();
}

void ScanFactory::preScan() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    // TODO no clue how to get around this
    for (json::iterator it = m_config["scan"]["prescan"]["FE-I4B"]["GlobalConfig"].begin(); it != m_config["scan"]["prescan"]["FE-I4B"]["GlobalConfig"].end(); ++it) {
        g_bk->globalFe<Fei4>()->writeRegister(g_bk->globalFe<Fei4>()->regMap[it.key()], it.value());
    }
}

void ScanFactory::postScan() {

}

void ScanFactory::loadConfig(json &scanCfg) {
    m_config = scanCfg;
    std::cout << "Loading Scan:" << std::endl;
    
    std::string name = scanCfg["scan"]["name"];
    std::cout << "  Name: " << name << std::endl;

    std::cout << "  Number of Loops: " << scanCfg["scan"]["loops"].size() << std::endl;

    for (int i=0; i<scanCfg["scan"]["loops"].size(); i++) {
        std::cout << "  Loading Loop #" << i << std::endl;
        std::string loopAction = scanCfg["scan"]["loops"][i]["loopAction"];
        std::cout << "  Type: " << loopAction << std::endl;
        std::shared_ptr<LoopActionBase> action;
        if (loopAction.find("Std") != std::string::npos) {
            action.reset(StdDict::getLoopAction(loopAction));
        } else if (loopAction.find("Fei4") != std::string::npos) {
            action.reset(Fei4Dict::getLoopAction(loopAction));
        } else {
            std::cout << "### ERROR ### => Unknown Loop Action: " << loopAction << " ... skipping!" << std::endl;
        }

        if (!scanCfg["scan"]["loops"][i]["config"].empty()) {
            std::cout << "  Loading loop config ... " << std::endl;
            action->loadConfig(scanCfg["scan"]["loops"][i]["config"]);
        }
        this->addLoop(action);

        //std::cout << " Check config: " << std::endl;
        //json tCfg;
        //action->writeConfig(tCfg);
        //std::cout << std::setw(4) << tCfg;
    }
                    

}
