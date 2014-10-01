// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Fei4 Digital Scan
// # Comment: Nothing fancy
// ################################

#include "Fei4DigitalScan.h"

Fei4DigitalScan::Fei4DigitalScan(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawData> *data) : ScanBase(fe, tx, rx, data) {
    mask = MASK_32;
    dcMode = QUAD_DC;
    numOfTriggers = 100;
    triggerFrequency = 5e3;
    triggerDelay = 50;
    verbose = false;
}

// Initialize Loops
void Fei4DigitalScan::init() {
    // Loop 1: Mask Staging
    std::shared_ptr<Fei4MaskLoop> maskStaging;
    maskStaging->setVerbose(verbose);
    maskStaging->setMaskStage(mask);

    // Loop 2: Double Columns
    std::shared_ptr<Fei4DcLoop> dcLoop;
    dcLoop->setVerbose(verbose);
    dcLoop->setMode(dcMode);

    // Loop 3: Trigger
    std::shared_ptr<Fei4TriggerLoop> triggerLoop;
    triggerLoop->setVerbose(verbose);
    triggerLoop->setTrigCnt(numOfTriggers);
    triggerLoop->setTrigFreq(triggerFrequency);
    triggerLoop->setTrigDelay(triggerDelay);

    // Loop 4: Data gatherer
    std::shared_ptr<StdDataLoop> dataLoop;
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
void Fei4DigitalScan::configure() {
    g_fe->writeRegister(&Fei4::Trig_Count, 4);
    g_fe->writeRegister(&Fei4::Trig_Lat, 255-triggerDelay-1);
    g_fe->writeRegister(&Fei4::DigHitIn_Sel, 0x1);
    while(!g_tx->isCmdEmpty());
}
