// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Threshold Scan
// # Comment: Nothing fancy
// ################################

#include "Fei4GlobalThresholdTune.h"

#include "ScanFactory.h"

namespace Fei4ScansRegistry {
  using StdDict::registerScan;

  bool global_threshold_scan_registered =
    registerScan("tune_globalthreshold",
                 [](Bookkeeper *k) {
                   return std::unique_ptr<ScanBase>(new Fei4GlobalThresholdTune(k));
                 });
}

Fei4GlobalThresholdTune::Fei4GlobalThresholdTune(Bookkeeper *b) : ScanBase(b) {
    mask = MASK_16;
    dcMode = QUAD_DC;
    numOfTriggers = 200;
    triggerFrequency = 10e3;
    triggerDelay = 50;

    useScap = true;
    useLcap = true;

    target = g_bk->getTargetCharge();
    verbose = false;
}


// Initialize Loops
void Fei4GlobalThresholdTune::init() {
    // Loop 0: Feedback, start with max fine threshold
    std::shared_ptr<Fei4GlobalFeedback> fbLoop(new Fei4GlobalFeedback(&Fei4::Vthin_Fine));
    fbLoop->setStep(16);
    fbLoop->setMax(255);
    fbLoop->setVerbose(true);

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

    this->addLoop(fbLoop);
    this->addLoop(maskStaging);
    this->addLoop(dcLoop);
    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);

    engine.init();
}

// Do necessary pre-scan configuration
void Fei4GlobalThresholdTune::preScan() {
    // Global config
    g_tx->setCmdEnable(g_bk->getTxMask());
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Count, 12);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Lat, (255-triggerDelay)-4);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::CalPulseWidth, 20); // Longer than max ToT 
    while(!g_tx->isCmdEmpty())
        ;

    for(unsigned int k=0; k<g_bk->feList.size(); k++) {
        Fei4 *fe = dynamic_cast<Fei4*>(g_bk->feList[k]);
        // Set to single channel tx
        g_tx->setCmdEnable(0x1 << fe->getTxChannel());
        // Set specific pulser DAC
    	fe->writeRegister(&Fei4::PlsrDAC, fe->toVcal(target, useScap, useLcap));
        // Reset all TDACs
        // TODO do not if retune
        for (unsigned col=1; col<81; col++)
            for (unsigned row=1; row<337; row++)
                fe->setTDAC(col, row, 16);
        fe->configurePixels();
        while(!g_tx->isCmdEmpty())
            ;
    }
    g_tx->setCmdEnable(g_bk->getTxMask());
}
