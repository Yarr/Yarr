// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Noise Scan
// # Comment: Nothing fancy
// ################################

#include "Fei4NoiseScan.h"

#include "ScanFactory.h"

namespace Fei4ScansRegistry {
  using StdDict::registerScan;

  bool noise_scan_registered =
    registerScan("noisescan",
                 [](Bookkeeper *k) {
                   return std::unique_ptr<ScanBase>(new Fei4NoiseScan(k));
                 });
}

Fei4NoiseScan::Fei4NoiseScan(Bookkeeper *k) : ScanBase(k) {
    triggerFrequency = 1e4;
    triggerTime = 30;
    verbose = false;
}

// Initialize Loops
void Fei4NoiseScan::init() {
    // Loop 1: Trigger
    std::shared_ptr<Fei4TriggerLoop> triggerLoop(new Fei4TriggerLoop);
    triggerLoop->setVerbose(verbose);
    triggerLoop->setTrigFreq(triggerFrequency);
    triggerLoop->setTrigTime(triggerTime);
    triggerLoop->setTrigCnt(0); // Activated time mode
    triggerLoop->setNoInject();

    // Loop 2: Data gatherer
    std::shared_ptr<StdDataGatherer> dataLoop(new StdDataGatherer);
    dataLoop->setVerbose(verbose);
    dataLoop->connect(g_data);

    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);
    
    engine.init();
}

// Do necessary pre-scan configuration
void Fei4NoiseScan::preScan() {
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Lat, 235);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Count, 5);
    while(!g_tx->isCmdEmpty());
}

