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

}

void Fe65p2DigitalScan::preScan() {
    g_fe65p2->setLatency(60);
    g_fe65p2->setValue(&Fe65p2::TestHit, 0x1);
    g_fe65p2->setValue(&Fe65p2::Latency, 60);
    g_fe65p2->configureGlobal();
    while(g_tx->isCmdEmpty() == 0);
}
