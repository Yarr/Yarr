// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Factory
// # Comment: Depends on dictionary for FE
// ################################

#include "ScanFactory.h"

#include <iomanip>
#include <iostream>

#include "AllStdActions.h"
#include "ClassRegistry.h"

#include "logging.h"

namespace {
    auto sflog = logging::make_log("ScanFactory");
}

ScanFactory::ScanFactory(Bookkeeper *k, FeedbackClipboardMap *fb)
  : ScanBase(k), feedback(fb) {
}

void ScanFactory::init() {
    engine.init();
}

void ScanFactory::preScan() {
    sflog->info("Entering pre scan phase ...");
    for (unsigned id=0; id<g_bk->getNumOfEntries(); id ++) {
        FrontEnd *fe = g_bk->getEntry(id).fe;
        fe->clipRawData.reset();
    }

    g_tx->setCmdEnable(g_bk->getTxMask());
    // Load scan specific registers from config
    auto &config_list = m_config["scan"]["prescan"];
    for (json::iterator it = config_list.begin(); it != config_list.end(); ++it) {
        FrontEnd &fe = *g_bk->getGlobalFe();
        fe.writeNamedRegister(it.key(), it.value());
    }
    while(!g_tx->isCmdEmpty()){}

    if (g_bk->getTargetCharge() > 0) {
        for (unsigned id=0; id<g_bk->getNumOfEntries(); id ++) {
            FrontEnd *fe = g_bk->getEntry(id).fe;
            if(fe->getActive()) {
                // Enable single channel
                g_tx->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
                // Write parameter
                sflog->info("Target charge specified as: {}", g_bk->getTargetCharge());
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

void ScanFactory::loadConfig(const json &scanCfg) {
    m_config = scanCfg;
    sflog->info("Loading Scan:");
    
    std::string name = scanCfg["scan"]["name"];
    sflog->info("  Name: {}", name);

    sflog->info("  Number of Loops: {}", scanCfg["scan"]["loops"].size());

    for (unsigned int i=0; i<scanCfg["scan"]["loops"].size(); i++) {
        sflog->info("  Loading Loop #{}", i);
        std::string loopAction = scanCfg["scan"]["loops"][i]["loopAction"];
        sflog->info("   Type: {}", loopAction);

        std::shared_ptr<LoopActionBase> action = StdDict::getLoopAction(loopAction);

        if (action == nullptr) {
            sflog->error("Unknown Loop Action: {}  ... skipping!", loopAction);
            sflog->warn("Known ScanLoop actions:");
            for(auto &la: StdDict::listLoopActions()) {
              sflog->warn("   {}", la);
            }

            continue;
        }

        if (scanCfg["scan"]["loops"][i].contains("config")) {
            sflog->info("   Loading loop config ... ");
            action->loadConfig(scanCfg["scan"]["loops"][i]["config"]);
        }

        if(auto *fbGlobal = dynamic_cast<GlobalFeedbackReceiver*>(&*action)) {
            fbGlobal->connectClipboard(feedback);
        }
        if(auto *fbPixel = dynamic_cast<PixelFeedbackReceiver*>(&*action)) {
            fbPixel->connectClipboard(feedback);
        }

        //if(auto *trigLoop = dynamic_cast<StdTriggerAction*>(&*action))
        if(std::shared_ptr<StdTriggerAction> trigLoop = std::dynamic_pointer_cast<StdTriggerAction>(action)) {
            g_bk->setTriggerAction(trigLoop);
        }

        this->addLoop(action);

        json tCfg;
        action->writeConfig(tCfg);
        if (!tCfg.empty()) {
            std::stringstream ss;
            ss << tCfg;
            std::string line;
            while (std::getline(ss, line)) sflog->info("~~~ {}", line); //yes overkill, i know ..
        } else {
            sflog->warn("~~~ Config empty.");
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
            sflog->error("No Scan class matching '{}' found", name);
        }
        return result;
    }

    std::vector<std::string> listScans() {
        return registry().listClasses();
    }
}
