#include "scanstruct.h"

void CustomScan::addPreScanReg(Fei4RegHelper f, uint16_t i) {
    std::pair<Fei4RegHelper, uint16_t> myPair;
    myPair.first = f;
    myPair.second = i;

    preScanRegs.append(myPair);

    return;
}

//void CustomScan::addScanLoop(std::shared_ptr<StdDataLoop> p) {
//    addLoop(p);

//    return;
//}

//void CustomScan::addScanLoop(std::shared_ptr<Fei4DcLoop> p) {
//    addLoop(p);

//    return;
//}

//void CustomScan::addScanLoop(std::shared_ptr<Fei4MaskLoop> p) {
//    addLoop(p);

//    return;
//}

//void CustomScan::addScanLoop(std::shared_ptr<Fei4TriggerLoop> p) {
//    addLoop(p);

//    return;

//}

void CustomScan::addScanLoop(std::shared_ptr<LoopActionBase> l) {
    scanLoops.append(l);

    return;
}


void CustomScan::addPostScanReg(Fei4RegHelper f, uint16_t i) {
    std::pair<Fei4RegHelper, uint16_t> myPair;
    myPair.first = f;
    myPair.second = i;

    postScanRegs.append(myPair);

    return;
}

void CustomScan::clearPreScanRegs() {
    preScanRegs.clear();

    return;
}

void CustomScan::clearScanLoops() {
    scanLoops.clear();

    return;
}

void CustomScan::clearPostScanRegs() {
    postScanRegs.clear();

    return;
}

int CustomScan::preScanSize() {
    return preScanRegs.size();
}

int CustomScan::scanLoopsSize() {
    return scanLoops.size();
}

int CustomScan::postScanSize() {
    return postScanRegs.size();
}

void CustomScan::preScan() {
    std::pair<Fei4RegHelper, uint16_t> v;

    foreach (v, preScanRegs) {
        v.first.writeReg(g_fe, v.second);
        while(!(g_tx->isCmdEmpty())) {
/*            std::cout << "Writing...\n"*/;
        }
    }

    //clearPreScanRegs();

    return;
}

void CustomScan::init() {
    foreach (std::shared_ptr<LoopActionBase> p, scanLoops) {
        addLoop(p);
    }

    engine.init();

    //clearScanLoops();

    return;
}

void CustomScan::postScan() {
    std::pair<Fei4RegHelper, uint16_t> v;
    foreach (v, postScanRegs) {
//        v.first.writeReg(this->b->getFe(0), v.second);
        v.first.writeReg(dynamic_cast<Fei4*>(this->b->getLastFe()), v.second);
        while(!(g_tx->isCmdEmpty())) {
            ;
        }
    }

    //clearPostScanRegs();

    clearPreScanRegs();
    clearScanLoops();
    clearPostScanRegs();

    return;
}

bool CustomScan::isScanEmpty()
{
    if(preScanRegs.size() > 0) {return false;}
    if(scanLoops.size() > 0) {return false;}
    if(postScanRegs.size() > 0) {return false;}
    return true;
}

ClipBoard<RawDataContainer> * CustomScan::getGData() {
    return this->g_data;
}

