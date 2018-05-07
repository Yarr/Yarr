// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fe65p2 Digital Scan
// # Data: 2016-Mar-30
// # Comment: Nothing fancy
// ################################

#include "Fe65p2DigitalScan.h"

Fe65p2DigitalScan::Fe65p2DigitalScan(Bookkeeper *k) : ScanBase(k) {

}

void Fe65p2DigitalScan::init() {
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

    this->addLoop(maskStaging);
    this->addLoop(dcLoop);
    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);
    
    engine.init();
}

void Fe65p2DigitalScan::preScan() {
    g_bk->globalFe<Fe65p2>()->setLatency(60);
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::TestHit, 0x1);
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::Latency, 60);
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::Vthin1Dac, 255);
    g_bk->globalFe<Fe65p2>()->setValue(&Fe65p2::Vthin2Dac, 10);
    g_bk->globalFe<Fe65p2>()->configureGlobal();
    while(g_tx->isCmdEmpty() == 0);
}
