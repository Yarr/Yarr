// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Scan Base Class
// # Comment:
// ################################

#include "ScanBase.h"

ScanBase::ScanBase(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawData> *data) : engine(fe, tx, rx){
    g_fe = fe;
    g_tx = tx;
    g_rx = rx;
    g_data = data;
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

unsigned ScanBase::size() {
    return loops.size();
}

void ScanBase::addLoop(std::shared_ptr<LoopActionBase> l) {
    loops.push_back(l);
    engine.addAction(l);
}


