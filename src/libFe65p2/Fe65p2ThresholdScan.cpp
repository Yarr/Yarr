// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65p2 Threshold Scan
// # Data: 2016-May-16
// # Comment: Nothing fancy
// ################################

#include "Fe65p2ThresholdScan.h"

Fe65p2ThresholdScan::Fe65p2ThresholdScan(Bookkeeper *k) : ScanBase (k) {

}

void Fe65p2ThresholdScan::init() {
    
    // Loop 1: Mask Staging
    std::shared_ptr<Fe65p2MaskLoop> maskStaging(new Fe65p2MaskLoop);

    // Loop 2: Double Columns
    std::shared_ptr<Fe65p2QcLoop> dcLoop(new Fe65p2QcLoop);
    
    // Loop 3: Parameter Loop
    std::shared_ptr<Fe65p2ParameterLoop> parLoop(new Fe65p2ParameterLoop(&Fe65p2GlobalCfg::PlsrDac));
    parLoop->setRange(50, 350, 10);
    // Loop 4: Trigger
    std::shared_ptr<Fe65p2TriggerLoop> triggerLoop(new Fe65p2TriggerLoop);

    // Loop 5: Data gatherer
    std::shared_ptr<StdDataLoop> dataLoop(new StdDataLoop);
    dataLoop->setVerbose(false);
    dataLoop->connect(g_data);

    this->addLoop(maskStaging);
    this->addLoop(dcLoop);
    this->addLoop(parLoop);
    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);
    
    engine.init();
}

void Fe65p2ThresholdScan::preScan() {
    g_bk->globalFe<Fe65p2>()->setLatency(60+5);
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::TestHit, 0x0);
    g_bk->globalFe<Fe65p2>()->enAnaInj();
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::Latency, 60);
    g_bk->globalFe<Fe65p2>()->configureGlobal();
    for(unsigned i=0; i<16; i++) {
        g_bk->globalFe<Fe65p2>()->PixConf(i).setAll(0);
    }

    g_bk->globalFe<Fe65p2>()->configurePixels();
    while(g_tx->isCmdEmpty() == 0);
}

void Fe65p2ThresholdScan::postScan() {

}
