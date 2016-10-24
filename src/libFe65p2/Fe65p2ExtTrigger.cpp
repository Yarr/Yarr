// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65-P2 Ext Trigger
// # Comment: Nothing fancy
// #################################

#include "Fe65p2ExtTrigger.h"

Fe65p2ExtTrigger::Fe65p2ExtTrigger(Bookkeeper *k) : ScanBase(k) {
    triggerFrequency = 1e3;
    triggerTime = 3600;
    verbose = false;
}

// Initialize Loops
void Fe65p2ExtTrigger::init() {
    

    // Loop 1: Trigger
    std::shared_ptr<Fe65p2TriggerLoop> triggerLoop(new Fe65p2TriggerLoop);
    triggerLoop->setVerbose(verbose);
    triggerLoop->setTrigFreq(triggerFrequency);
    triggerLoop->setTrigTime(triggerTime);
    triggerLoop->setTrigCnt(0); // Activated time mode
    triggerLoop->setNoInject();
    triggerLoop->setExtTrigger();

    // Loop 2: Data gatherer
    std::shared_ptr<StdDataGatherer> dataLoop(new StdDataGatherer);
    dataLoop->setVerbose(verbose);
    dataLoop->connect(g_data);

    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);
    
    engine.init();
}

// Do necessary pre-scan configuration
void Fe65p2ExtTrigger::preScan() {
    //g_fe65p2->setValue(&Fe65p2::TrigCount, 3);
    //g_fe65p2->setValue(&Fe65p2::Latency, 82);
    //g_fe65p2->configureGlobal();
    while(!g_tx->isCmdEmpty());
}

