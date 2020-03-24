// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Analog Scan
// # Comment: Nothing fancy
// ################################

#include "Fei4AnalogScan.h"

#include "ScanFactory.h"

namespace Fei4ScansRegistry {
  using StdDict::registerScan;

  bool analogue_scan_registered =
    registerScan("analogscan",
                 [](Bookkeeper *k) {
                   return std::unique_ptr<ScanBase>(new Fei4AnalogScan(k));
                 });
}

Fei4AnalogScan::Fei4AnalogScan(Bookkeeper *k) : ScanBase(k) {
    mask = MASK_32;
    dcMode = QUAD_DC;
    numOfTriggers = 100;
    triggerFrequency = 20e3;
    triggerDelay = 50;
}

// Initialize Loops
void Fei4AnalogScan::init() {
    // Loop 1: Mask Staging
    std::shared_ptr<Fei4MaskLoop> maskStaging(new Fei4MaskLoop);
    maskStaging->setMaskStage(mask);
    maskStaging->setScap(true);
    maskStaging->setLcap(true);

    // Loop 2: Double Columns
    std::shared_ptr<Fei4DcLoop> dcLoop(new Fei4DcLoop);
    dcLoop->setMode(dcMode);

    // Loop 3: Trigger
    std::shared_ptr<Fei4TriggerLoop> triggerLoop(new Fei4TriggerLoop);
    triggerLoop->setTrigCnt(numOfTriggers);
    triggerLoop->setTrigFreq(triggerFrequency);
    triggerLoop->setTrigDelay(triggerDelay);

    // Loop 4: Data gatherer
    std::shared_ptr<StdDataLoop> dataLoop(new StdDataLoop);
    dataLoop->connect(g_data);

    this->addLoop(maskStaging);
    this->addLoop(dcLoop);
    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);

    engine.init();
}

// Do necessary pre-scan configuration
void Fei4AnalogScan::preScan() {
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Count, 12);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Lat, (255-triggerDelay)-5);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::PlsrDAC, 300);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::CalPulseWidth, 20); // Longer than max ToT 
    while(!g_tx->isCmdEmpty());
}
