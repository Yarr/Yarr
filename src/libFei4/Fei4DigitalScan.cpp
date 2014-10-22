// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Digital Scan
// # Comment: Nothing fancy
// ################################

#include "Fei4DigitalScan.h"

Fei4DigitalScan::Fei4DigitalScan(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawData> *data) : ScanBase(fe, tx, rx, data) {
    mask = MASK_16;
    dcMode = QUAD_DC;
    numOfTriggers = 100;
    triggerFrequency = 12e3;
    triggerDelay = 50;
    verbose = false;
}

// Initialize Loops
void Fei4DigitalScan::init() {
    // Loop 1: Mask Staging
    std::shared_ptr<Fei4MaskLoop> maskStaging(new Fei4MaskLoop);
    maskStaging->setVerbose(verbose);
    maskStaging->setMaskStage(mask);

    // Loop 2: Double Columns
    std::shared_ptr<Fei4DcLoop> dcLoop(new Fei4DcLoop);
    dcLoop->setVerbose(verbose);
    dcLoop->setMode(dcMode);

    // Loop 3: Trigger
    std::shared_ptr<Fei4TriggerLoop> triggerLoop(new Fei4TriggerLoop);
    triggerLoop->setVerbose(verbose);
    triggerLoop->setTrigCnt(numOfTriggers);
    triggerLoop->setTrigFreq(triggerFrequency);
    triggerLoop->setTrigDelay(triggerDelay);

    // Loop 4: Data gatherer
    std::shared_ptr<StdDataLoop> dataLoop(new StdDataLoop);
    dataLoop->setVerbose(verbose);
    dataLoop->connect(g_data);

    loops.push_back(maskStaging);
    loops.push_back(dcLoop);
    loops.push_back(triggerLoop);
    loops.push_back(dataLoop);

    engine.addAction(maskStaging);
    engine.addAction(dcLoop);
    engine.addAction(triggerLoop);
    engine.addAction(dataLoop);

    engine.init();
}

// Do necessary pre-scan configuration
void Fei4DigitalScan::preScan() {
    g_fe->writeRegister(&Fei4::Trig_Count, 10);
    g_fe->writeRegister(&Fei4::Trig_Lat, 255-triggerDelay-4);
    g_fe->writeRegister(&Fei4::DigHitIn_Sel, 0x1);
    g_fe->writeRegister(&Fei4::Vthin_Coarse, 200);
    while(g_tx->isCmdEmpty() == 0);
}
