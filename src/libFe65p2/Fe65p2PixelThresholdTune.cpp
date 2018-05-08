// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65p2 Analog Scan
// # Data: 2016-Mar-30
// # Comment: Nothing fancy
// ################################

#include "Fe65p2PixelThresholdTune.h"

Fe65p2PixelThresholdTune::Fe65p2PixelThresholdTune(Bookkeeper *k) : ScanBase(k) {

}

void Fe65p2PixelThresholdTune::init() {
    // Loop1: Feedback llop
    std::shared_ptr<Fe65p2PixelFeedback> fbLoop(new Fe65p2PixelFeedback());
    
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

void Fe65p2PixelThresholdTune::preScan() {
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::PlsrDac, g_bk->globalFe<Fe65p2>()->toVcal(g_bk->getTargetCharge())); // Set high injection
    g_bk->globalFe<Fe65p2>()->configDac();
    
    g_bk->globalFe<Fe65p2>()->setLatency(60+5);
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::TestHit, 0x0);
    g_bk->globalFe<Fe65p2>()->enAnaInj();
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::Latency, 60);
    g_bk->globalFe<Fe65p2>()->configureGlobal();
    // Reset all TDACs
    for(unsigned i=0; i<16; i++) {
        g_bk->globalFe<Fe65p2>()->TDAC(i).setAll(0);
        g_bk->globalFe<Fe65p2>()->Sign(i).setAll(1);
    }

    g_bk->globalFe<Fe65p2>()->configurePixels();
    while(g_tx->isCmdEmpty() == 0);
}
