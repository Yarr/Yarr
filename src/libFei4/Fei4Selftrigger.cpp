// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Selftrigger
// # Comment: Nothing fancy
// ################################

#include "Fei4Selftrigger.h"

Fei4Selftrigger::Fei4Selftrigger(Bookkeeper *k) : ScanBase(k) {
    triggerFrequency = 1e4;
    triggerTime = 30;
    verbose = false;
}

// Initialize Loops
void Fei4Selftrigger::init() {
    // Loop 1: Trigger
    std::shared_ptr<Fei4TriggerLoop> triggerLoop(new Fei4TriggerLoop);
    triggerLoop->setVerbose(verbose);
    triggerLoop->setTrigFreq(triggerFrequency);
    triggerLoop->setTrigTime(triggerTime);
    triggerLoop->setTrigCnt(0); // Activated time mode
    triggerLoop->setNoWord(); //Fei4 selftrigger just sends data

    // Loop 2: Data gatherer
    std::shared_ptr<StdDataGatherer> dataLoop(new StdDataGatherer);
    dataLoop->setVerbose(verbose);
    dataLoop->connect(g_data);

    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);
    
    engine.init();
}

// Do necessary pre-scan configuration
void Fei4Selftrigger::preScan() {
    g_fe->writeRegister(&Fei4::Trig_Lat, 235);
    g_fe->writeRegister(&Fei4::HitOr, 1);
    g_fe->writeRegister(&Fei4::Trig_Count, 5);
    while(!g_tx->isCmdEmpty());
}

