// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Threshold Scan
// # Comment: Nothing fancy
// ################################

#include "Fei4PixelThresholdTune.h"

#include "ScanFactory.h"

namespace Fei4ScansRegistry {
  using StdDict::registerScan;

  bool pixel_threshold_scan_registered =
    registerScan("tune_pixelthreshold",
                 [](Bookkeeper *k) {
                   return std::unique_ptr<ScanBase>(new Fei4PixelThresholdTune(k));
                 });
}

Fei4PixelThresholdTune::Fei4PixelThresholdTune(Bookkeeper *b) : ScanBase(b) {
    mask = MASK_16;
    dcMode = QUAD_DC;
    numOfTriggers = 200;
    triggerFrequency = 10e3;
    triggerDelay = 50;
    
    useScap = true;
    useLcap = true;

    target = g_bk->getTargetCharge();
}

// Initialize Loops
void Fei4PixelThresholdTune::init() {
    // Loop 0: Feedback
    std::shared_ptr<Fei4PixelFeedback> fbLoop(new Fei4PixelFeedback(TDAC_FB));

    // Loop 1: Mask Staging
    std::shared_ptr<Fei4MaskLoop> maskStaging(new Fei4MaskLoop);
    maskStaging->setMaskStage(mask);
    maskStaging->setScap(useScap);
    maskStaging->setLcap(useLcap);
    
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

    this->addLoop(fbLoop);
    this->addLoop(maskStaging);
    this->addLoop(dcLoop);
    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);

    engine.init();
}

// Do necessary pre-scan configuration
void Fei4PixelThresholdTune::preScan() {
    // Global config
    g_tx->setCmdEnable(g_bk->getTxMask());
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Count, 12);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::Trig_Lat, (255-triggerDelay)-4);
    g_bk->globalFe<Fei4>()->writeRegister(&Fei4::CalPulseWidth, 20); // Longer than max ToT 
    while(!g_tx->isCmdEmpty())
        ;

    for(unsigned id=0; id<g_bk->getNumOfEntries(); id++) {
        Fei4 *fe = dynamic_cast<Fei4*>(g_bk->getFe(id));
        if (fe->isActive()) {
            // Set to single channel tx
            g_tx->setCmdEnable(fe->getTxChannel());
            // Set specific pulser DAC
            fe->writeRegister(&Fei4::PlsrDAC, fe->toVcal(target, useScap, useLcap));
            // Reset all TDACs
            for (unsigned col=1; col<81; col++)
                for (unsigned row=1; row<337; row++)
                    fe->setTDAC(col, row, 16);
            while(!g_tx->isCmdEmpty())
                ;
        }
    }
    g_tx->setCmdEnable(g_bk->getTxMask());
}
