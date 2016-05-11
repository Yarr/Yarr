#include "Fei4CrossTalkScan.h"

Fei4CrossTalkScan::Fei4CrossTalkScan(Fei4 *fe, TxCore *tx, RxCore *rx, ClipBoard<RawDataContainer> *data) : ScanBase(fe, tx, rx, data) {
    mask = MASK_32;
    dcMode = QUAD_DC;
    numOfTriggers = 100;
    triggerFrequency = 20e3;
    triggerDelay = 50;
    verbose = false;
}

Fei4CrossTalkScan::Fei4CrossTalkScan(Bookkeeper *k) : ScanBase(k) {
    mask = MASK_32;
    dcMode = QUAD_DC;
    numOfTriggers = 100;
    triggerFrequency = 20e3;
    triggerDelay = 50;
    verbose = false;
}

// Initialize Loops
void Fei4CrossTalkScan::init() {
    // Loop 1: Mask Staging
    std::shared_ptr<Fei4MaskLoop> maskStaging(new Fei4MaskLoop);
    maskStaging->setVerbose(verbose);
    maskStaging->setMaskStage(mask);
    maskStaging->setScap(true);
    maskStaging->setLcap(true);

    // Loop 2: Double Columns
    std::shared_ptr<Fei4DcLoop> dcLoop(new Fei4DcLoop);
    dcLoop->setVerbose(verbose);
    dcLoop->setMode(dcMode);

    // Loop 3: Injection
    std::shared_ptr<Fei4TriggerLoop> injectLoop(new Fei4TriggerLoop);
    injectLoop->setVerbose(verbose);
    injectLoop->setTrigCnt(numOfTriggers);
    injectLoop->setTrigFreq(triggerFrequency);
    injectLoop->setOnlyInject();

    // Loop 4: Switch pixels
    std::shared_ptr<Fei4CrossTalkLoop> crossLoop(new Fei4CrossTalkLoop);

    // Loop 5: Trigger
    std::shared_ptr<Fei4TriggerLoop> triggerLoop(new Fei4TriggerLoop);
    triggerLoop->setVerbose(verbose);
    triggerLoop->setTrigCnt(numOfTriggers);
    triggerLoop->setTrigFreq(triggerFrequency);
    triggerLoop->setTrigDelay(triggerDelay);

    // Loop 6: Data gatherer
    std::shared_ptr<StdDataLoop> dataLoop(new StdDataLoop);
    dataLoop->setVerbose(verbose);
    dataLoop->connect(g_data);

    this->addLoop(maskStaging);
    this->addLoop(dcLoop);
    this->addLoop(injectLoop);
    this->addLoop(crossLoop);
    this->addLoop(triggerLoop);
    this->addLoop(dataLoop);

    engine.init();
}

// Do necessary pre-scan configuration
void Fei4CrossTalkScan::preScan() {
    g_fe->writeRegister(&Fei4::Trig_Count, 12);
    g_fe->writeRegister(&Fei4::Trig_Lat, (255-triggerDelay)-5);
    g_fe->writeRegister(&Fei4::PlsrDAC, 300);
    g_fe->writeRegister(&Fei4::CalPulseWidth, 20); // Longer than max ToT 
    while(!g_tx->isCmdEmpty());
}
