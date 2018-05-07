// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Base Class
// # Comment:
// ################################

#include "ScanBase.h"

ScanBase::ScanBase(Bookkeeper *k) : engine(k) {
    g_data = &k->rawData;
    g_bk = k;
    g_tx = k->tx;
    g_rx = k->rx;
}

void ScanBase::run() {
    engine.execute();
    engine.end();
}

std::shared_ptr<LoopActionBase> ScanBase::getLoop(unsigned n) {
    return loops[n];
}

std::shared_ptr<LoopActionBase> ScanBase::operator[](unsigned n) {
    return loops[n];
}

std::shared_ptr<LoopActionBase> ScanBase::operator[](std::type_index t) {
    return loopMap[t];
}

unsigned ScanBase::size() {
    return loops.size();
}

void ScanBase::addLoop(std::shared_ptr<LoopActionBase> l) {
    loops.push_back(l);
    engine.addAction(l);
    loopMap[l->type()] = l;
}


