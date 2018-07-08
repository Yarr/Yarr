// #################################
// # Author: Eunchong Kim
// # Email: eunchong.kim at cern.ch
// # Project: masspro on Yarr
// # Description: Fei4 Charge vs. ToT Scan
// # Comment: Nothing fancy
// ################################

#include "Fei4ChargeVsTot.h"

#include "ScanFactory.h"

namespace Fei4ScansRegistry {
  using StdDict::registerScan;

  bool chargevstot_scan_registered =
    registerScan("chargevstot",
                 [](Bookkeeper *k) {
                   return std::unique_ptr<ScanBase>(new Fei4ChargeVsTot(k));
                 });
}

Fei4ChargeVsTot::Fei4ChargeVsTot(Bookkeeper *b) : ScanBase(b) {
    mask = MASK_16;
    dcMode = QUAD_DC;
    numOfTriggers = 100;
    triggerFrequency = 10e3;
    triggerDelay = 50;
    useScap = true;
    useLcap = true;
    minVcal = 10;
    maxVcal = 560;
    stepVcal = 10;

    targetCharge = (double)b->getTargetCharge();

    verbose = false;
}

// Initialize Loops
void Fei4ChargeVsTot::init() {
    // Loop 1: Mask Staging
    std::shared_ptr<Fei4MaskLoop> maskStaging(new Fei4MaskLoop);
    maskStaging->setVerbose(verbose);
    maskStaging->setMaskStage(mask);
    maskStaging->setScap(useScap);
    maskStaging->setLcap(useLcap);
    
    // Loop 2: Double Columns
    std::shared_ptr<Fei4DcLoop> dcLoop(new Fei4DcLoop);
    dcLoop->setVerbose(verbose);
    dcLoop->setMode(dcMode);

    // Loop 1: Parameter Loop
    std::shared_ptr<Fei4ParameterLoop> parLoop(new Fei4ParameterLoop(&Fei4::PlsrDAC));
    parLoop->setRange(minVcal, maxVcal, stepVcal);
    parLoop->setVerbose(false);

    // Loop 3: Trigger
    std::shared_ptr<Fei4TriggerLoop> triggerLoop(new Fei4TriggerLoop);
    triggerLoop->setVerbose(verbose);
    triggerLoop->setTrigCnt(numOfTriggers);
    triggerLoop->setTrigFreq(triggerFrequency);
    triggerLoop->setTrigDelay(triggerDelay);

    // Loop 4: Data gatherer
    std::shared_ptr<StdDataLoop> dataLoop(new StdDataLoop);
    dataLoop->setVerbose(verbose);
    dataLoop->connect(g_data);

    this->addLoop(maskStaging);
    this->addLoop(dcLoop);
    this->addLoop(parLoop);
    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);

    engine.init();
}

// Do necessary pre-scan configuration
void Fei4ChargeVsTot::preScan() {
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Count, 12);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Lat, (255-triggerDelay)-4);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::PlsrDAC, 300);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::CalPulseWidth, 20); // Longer than max ToT 
    while(!g_tx->isCmdEmpty());
}
