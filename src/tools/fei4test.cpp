#include <iostream>
#include <stdint.h>
#include <fstream>
#include <string>

#include "SpecController.h"
#include "TxCore.h"
#include "Fei4.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "BenchmarkTools.h"
#include "Fei4EventData.h"
#include "Fei4DataProcessor.h"

#include "LoopEngine.h"

#include "AllStdActions.h"
#include "AllFei4Actions.h"


#define RX_ADDR (0x2 << 14)
#define RX_BRIDGE (0x3 << 14)

#define RX_START_ADDR 0x0
#define RX_DATA_COUNT 0x1
#define RX_LOOPBACK 0x2
#define RX_DATA_RATE 0x3
#define RX_LOOP_FIFO 0x4

int main(void) {
    // Init
    SpecController spec(0);
    TxCore tx(&spec);
    RxCore rx(&spec);

    Fei4 g_fe(&tx, 0);
    Fei4 fe(&tx, 0);
    
    ClipBoard<RawData> clipRaw;
    ClipBoard<Fei4Data> clipEvent;

    std::string tmp;

    std::cout << "### Initilizing Loop Engine ###" << std::endl;
    // Init Loop Engine
    LoopEngine eng(&g_fe, &tx, &rx);
    
    // Init specific loops
    Fei4MaskLoop maskStaging;
    maskStaging.setVerbose(true);
    maskStaging.setMaskStage(MASK_32);
    eng.addAction(std::make_shared<Fei4MaskLoop>(maskStaging));

    Fei4DcLoop dcLoop;
    dcLoop.setVerbose(true);
    dcLoop.setMode(QUAD_DC);
    eng.addAction(std::make_shared<Fei4DcLoop>(dcLoop));

    Fei4TriggerLoop triggerLoop;
    triggerLoop.setVerbose(true);
    triggerLoop.setTrigCnt(50);
    triggerLoop.setTrigFreq(5e3);
    //triggerLoop.setTrigDelay(50);
    eng.addAction(std::make_shared<Fei4TriggerLoop>(triggerLoop));

    StdDataLoop dataLoop;
    dataLoop.setVerbose(true);
    dataLoop.connect(&clipRaw);
    eng.addAction(std::make_shared<StdDataLoop>(dataLoop));

    std::cin >> tmp;

    eng.init();

    // Config
    std::cout << "### Sending Configuration ###" << std::endl;
    tx.setCmdEnable(0x1);
    fe.setRunMode(false);
    fe.configure();
    fe.writeRegister(&Fei4::Trig_Count, 4);
    fe.writeRegister(&Fei4::Trig_Lat, 255-triggerLoop.getTrigDelay()-1);
    fe.writeRegister(&Fei4::DigHitIn_Sel, 0x1);
    while(!tx.isCmdEmpty());
    rx.setRxEnable(0x1);
    std::cin >> tmp;

    // Start engine
    std::cout << "### Starting Engine ###" << std::endl;
    eng.execute();


    // Stop Engine
    std::cout << "### Stopping Engine ###" << std::endl;
    eng.end();

#if 0
    uint32_t trigger_word[4] = {0x00, 0x00, 0x001d ,0x1640}; // 255 - 36 = 219 -> TrigLat

    // Setup Trigger
    tx.setTrigConfig(INT_COUNT);
    tx.setTrigFreq(5e3); // 100kHz
    tx.setTrigCnt(50);
    tx.setTrigWordLength(64);
    tx.setTrigWord(trigger_word);

    // Setup FE
    fe.setRunMode(false);
    fe.writeRegister(&Fei4::Trig_Count, 3);
    fe.writeRegister(&Fei4::Trig_Lat, 222);

    // Init Mask Staging
    fe.writeRegister(&Fei4::Colpr_Mode, 0x3);
    fe.writeRegister(&Fei4::Colpr_Addr, 0x0);
    fe.initMask(MASK_32);
    fe.loadIntoPixel(0x1);
    fe.writeRegister(&Fei4::DigHitIn_Sel, 0x1);

    int count = 0;
    for (int i=0; i<32; i++) {
        std::cout << "Mask Stage: " << count << std::endl;
        fe.setRunMode(true);
        while(!tx.isCmdEmpty());
        std::cout << "Starting trigger " << std::endl;
        tx.setTrigEnable(0x1);
        std::cout << "Done " << std::endl;
        unsigned done = 0;
        unsigned dma_addr = 0;
        unsigned dma_count = 0;
        unsigned rate = 0;
        while (done == 0 || dma_count == 0) {
            dma_count = 0;
            dma_addr = 0;
            rate = spec.readSingle(RX_BRIDGE | RX_DATA_RATE);
            dma_addr = spec.readSingle(RX_BRIDGE | RX_START_ADDR);
            dma_count = spec.readSingle(RX_BRIDGE | RX_DATA_COUNT);
            if (dma_count > 0 && dma_count != 0xFFFFFFFF) {
                done = tx.isTrigDone();
                std::cout << "Some data " << dma_count << std::endl;
                uint32_t *buf = new uint32_t[dma_count];
                double readTime = BenchmarkTools::measureReadTime(&spec, dma_addr, buf, dma_count, 1);
                if (readTime < 0) {
                    std::cout << "DMA Failed!" << std::endl;
                    return -1;
                }
                clipRaw.pushData(new RawData(dma_addr, buf, dma_count));
            }
        }
        tx.setTrigEnable(0x0);
        std::cout << "Stopped trigger" << std::endl;
        fe.setRunMode(false);
        fe.shiftMask();
        fe.loadIntoPixel(0x1);
        count++;
    }
    spec.writeSingle(RX_ADDR | 0x0, 0x0);
#endif

    std::cout << "### Disabling RX ###" << std::endl;
    tx.setCmdEnable(0x0);
    rx.setRxEnable(0x0);
    
    std::cout << "### Analyzing data ###" << std::endl;
    Fei4DataProcessor proc;
    proc.connect(&clipRaw, &clipEvent);
    proc.process();
    unsigned i = 0;
    std::string filename = "digitalscan.txt";
    // Recreate File
    std::fstream file(filename, std::fstream::trunc);
    file.close();
    std::cout << "Writing: " << clipEvent.size() << " files ..." << std::endl;
    while(!clipEvent.empty()) {
        Fei4Data *data = clipEvent.popData();
        data->toFile(filename);
        i++;
        delete data;
    }
    std::cout << "... done!" << std::endl;
}
