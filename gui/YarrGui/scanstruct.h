#ifndef SCANSTRUCT_H
#define SCANSTRUCT_H

#include <array>
#include <memory>
#include <stdint.h>
#include <unistd.h>
#include <utility>

#include <QList>

#include "AllFei4Actions.h"
#include "AllStdActions.h"
#include "Bookkeeper.h"
#include "ClipBoard.h"
#include "Fei4.h"
#include "fei4reghelper.h"
#include "LoopActionBase.h"
#include "RawData.h"
#include "SpecRxCore.h"
#include "ScanBase.h"
#include "SpecTxCore.h"

class CustomScan : public ScanBase {
public:
    CustomScan() : ScanBase(nullptr, nullptr, nullptr, nullptr) {for(unsigned int i; i < 8; i++) bA.at(i) = false;}
    CustomScan(Fei4 * p1, SpecTxCore * p2, SpecRxCore * p3, ClipBoard<RawDataContainer> * p4)
        : ScanBase(p1, p2, p3, p4) {for(unsigned int i = 0; i < 8; i++) bA.at(i) = false;}
    CustomScan(Bookkeeper * bk) : ScanBase(bk) {for(unsigned int i = 0; i < 8; i++) bA.at(i) = false;}
    ~CustomScan() {}

    void addPreScanReg(Fei4RegHelper f, uint16_t i);
    void addScanLoop(std::shared_ptr<LoopActionBase>);
//    void addScanLoop(std::shared_ptr<StdDataLoop> p);
//    void addScanLoop(std::shared_ptr<Fei4DcLoop> p);
//    void addScanLoop(std::shared_ptr<Fei4MaskLoop> p);
//    void addScanLoop(std::shared_ptr<Fei4TriggerLoop> p);
    void addPostScanReg(Fei4RegHelper f, uint16_t i);

    void clearPreScanRegs();
    void clearScanLoops();
    void clearPostScanRegs();

    int preScanSize();
    int scanLoopsSize();
    int postScanSize();

    void init();
    void preScan();
    void postScan();

    bool isScanEmpty();

    ClipBoard<RawDataContainer> * getGData();

    std::array<bool, 8> bA;         //used to add the correct histograms and analysis algorithms

private:
    QList<std::pair<Fei4RegHelper, uint16_t> > preScanRegs;
    QList<std::shared_ptr<LoopActionBase> > scanLoops;
    QList<std::pair<Fei4RegHelper, uint16_t> > postScanRegs;
};

#endif // SCANSTRUCT_H
