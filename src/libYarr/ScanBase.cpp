// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Base Class
// # Comment:
// ################################

#include "ScanBase.h"

ScanBase::ScanBase(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawDataContainer> *data) : engine(fe, tx, rx){
    g_fe = fe;
    g_tx = tx;
    g_rx = rx;
    g_data = data;
}

ScanBase::ScanBase(Bookkeeper *k) : engine(k) {
    g_fe = k->g_fe;
    g_tx = k->tx;
    g_rx = k->rx;
    g_data = &k->rawData;
    b = k;
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


