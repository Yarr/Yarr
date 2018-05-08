// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65p2 Analog Scan
// # Data: 2016-Mar-30
// # Comment: Nothing fancy
// ################################

#include "Fe65p2GlobalThresholdTune.h"

Fe65p2GlobalThresholdTune::Fe65p2GlobalThresholdTune(Bookkeeper *k) : ScanBase(k) {

}

void Fe65p2GlobalThresholdTune::init() {
    // Loop1: Feedback llop
    std::shared_ptr<Fe65p2GlobalFeedback> fbLoop(new Fe65p2GlobalFeedback(&Fe65p2::Vthin1Dac));
    fbLoop->setMax(150);
    fbLoop->setMin(0);
    fbLoop->setStep(8);
    
    // Loop 1: Mask Staging
    std::shared_ptr<Fe65p2MaskLoop> maskStaging(new Fe65p2MaskLoop);

    // Loop 2: Double Columns
    std::shared_ptr<Fe65p2QcLoop> dcLoop(new Fe65p2QcLoop);
    
    // Loop 3: Trigger
    std::shared_ptr<Fe65p2TriggerLoop> triggerLoop(new Fe65p2TriggerLoop);

    // Loop 4: Data gatherer
    std::shared_ptr<StdDataLoop> dataLoop(new StdDataLoop);
    dataLoop->setVerbose(false);
    dataLoop->connect(g_data);

    this->addLoop(fbLoop);
    this->addLoop(maskStaging);
    this->addLoop(dcLoop);
    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);
    
    engine.init();
}

void Fe65p2GlobalThresholdTune::preScan() {
    g_bk->globalFe<Fe65p2>()->setLatency(60+5);
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::TestHit, 0x0);
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::Vthin1Dac, 255); // Set Threshold not too low, not too high
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::PlsrDac, g_bk->globalFe<Fe65p2>()->toVcal(g_bk->getTargetCharge())); // Set high injection
    g_bk->globalFe<Fe65p2>()->configDac();
    g_bk->globalFe<Fe65p2>()->enAnaInj();
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::Latency, 60);
    g_bk->globalFe<Fe65p2>()->configureGlobal();
    // Reset all TDACs
    for(unsigned i=0; i<16; i++) {
        //g_bk->globalFe<Fe65p2>()->TDAC(i).setAll(0);
        //g_bk->globalFe<Fe65p2>()->Sign(i).setAll(0);
    }

    g_bk->globalFe<Fe65p2>()->configurePixels();
    while(g_tx->isCmdEmpty() == 0);
}
