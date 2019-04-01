// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Factory
// # Comment: Depends on dictionary for FE
// ################################

#include "ScanFactory.h"

#include <iomanip>

#include "AllStdActions.h"
#include "ClassRegistry.h"

ScanFactory::ScanFactory(Bookkeeper *k) : ScanBase(k) {
}

void ScanFactory::init() {
    // TODO I don't like this, we assume the innermost loops get the data
    dynamic_cast<StdDataAction*>((this->getLoop(this->size()-1)).get())->connect(g_data);    
    
    engine.init();
}

void ScanFactory::preScan() {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    g_tx->setCmdEnable(g_bk->getTxMask());
    // Load scan specific registers from config
    auto &config_list = m_config["scan"]["prescan"];
    for (json::iterator it = config_list.begin(); it != config_list.end(); ++it) {
        FrontEnd &fe = *g_bk->getGlobalFe();
        fe.writeNamedRegister(it.key(), it.value());
    }
    while(!g_tx->isCmdEmpty()){}

    if (g_bk->getTargetCharge() > 0) {
        for (auto *fe : g_bk->feList) {
            if(fe->getActive()) {
                // Enable single channel
                g_tx->setCmdEnable(1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
                // Write parameter
                fe->setInjCharge(g_bk->getTargetCharge(), true, true); // TODO need sCap/lCap for FEI4
                while(!g_tx->isCmdEmpty()){}
            }
        }
        // Reset CMD mask
        g_tx->setCmdEnable(g_bk->getTxMask());
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

    for (unsigned int i=0; i<scanCfg["scan"]["loops"].size(); i++) {
        std::cout << "  Loading Loop #" << i << std::endl;
        std::string loopAction = scanCfg["scan"]["loops"][i]["loopAction"];
        std::cout << "  Type: " << loopAction << std::endl;
        std::shared_ptr<LoopActionBase> action
          = StdDict::getLoopAction(loopAction);

        if (action == nullptr) {
            std::cout << "### ERROR ### => Unknown Loop Action: " << loopAction << " ... skipping!" << std::endl;

            std::cout << " Known ScanLoop actions:\n";
            for(auto &la: StdDict::listLoopActions()) {
              std::cout << "   " << la << std::endl;
            }

            continue;
        }

        if (!scanCfg["scan"]["loops"][i]["config"].empty()) {
            std::cout << "  Loading loop config ... " << std::endl;
            action->loadConfig(scanCfg["scan"]["loops"][i]["config"]);
        }
        this->addLoop(action);

        std::cout << " Check config: " << std::endl;
        json tCfg;
        action->writeConfig(tCfg);
        if (!tCfg.empty()) {
            std::cout << "~~~~~~~~~~" << std::endl;
            std::cout << std::setw(4) << tCfg << std::endl;
            std::cout << "~~~~~~~~~~" << std::endl;
        } else {
            std::cout << "~~~~~~~~~~" << std::endl;
            std::cout << "  Config empty." << std::endl;
            std::cout << "~~~~~~~~~~" << std::endl;
        }
    }
                    

}

namespace StdDict {
    typedef ClassRegistry<ScanBase, Bookkeeper *> OurRegistry;

    static OurRegistry &registry() {
        static OurRegistry instance;
        return instance;
    }

    bool registerScan(std::string name,
                      std::function<std::unique_ptr<ScanBase>(Bookkeeper *k)> f)
    {
      return registry().registerClass(name, f);

    }

    std::unique_ptr<ScanBase> getScan(std::string name, Bookkeeper *b) {
        auto result = registry().makeClass(name, b);
        if(result == nullptr) {
            std::cout << "No Scan class matching '" << name << "' found\n";
        }
        return result;
    }

    std::vector<std::string> listScans() {
        return registry().listClasses();
    }
}
