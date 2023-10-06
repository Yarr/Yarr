// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Base Class
// # Comment:
// ################################

#include "ScanBase.h"

ScanBase::ScanBase(Bookkeeper *k) : engine(k) {
    g_bk = k;
    g_tx = k->tx;
    g_rx = k->rx;
}

void ScanBase::run() {
    engine.execute();
    engine.end();
}

const LoopActionBaseInfo *ScanBase::getLoop(unsigned n) const {
    return loops[n].get();
}

unsigned ScanBase::size() {
    return loops.size();
}

void ScanBase::addLoop(std::shared_ptr<LoopActionBase> l) {
    loops.push_back(l);
    engine.addAction(l);
}


