#include <iostream>
#include <stdint.h>

#include "SpecController.h"
#include "TxCore.h"
#include "Fei4.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "BenchmarkTools.h"
#include "Fei4EventData.h"
#include "Fei4DataProcessor.h"

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
    Fei4 fe(&tx, 0, 0);
    ClipBoard<RawData> clipRaw;
    ClipBoard<Fei4Data> clipEvent;
    
    // Config
    std::cout << "### Sending Configuration ###" << std::endl;
    tx.setCmdEnable(0x1);
    fe.sendConfig();

    uint32_t trigger_word[4] = {0x00, 0x00, 0x001d ,0x1680}; // 255 - 36 = 219 -> TrigLat

    // Setup Trigger
    tx.setTrigConfig(INT_COUNT);
    tx.setTrigFreq(100e3); // 100kHz
    tx.setTrigCnt(100);
    tx.setTrigWordLength(64);
    tx.setTrigWord(trigger_word);

    // Setup FE
    fe.setRunMode(false);
    fe.writeRegister(&Fei4::Trig_Count, 4);
    fe.writeRegister(&Fei4::Trig_Lat, 220);

    // Init Mask Staging
    fe.writeRegister(&Fei4::Colpr_Mode, 0x3); // all
    fe.writeRegister(&Fei4::Colpr_Addr, 0x0);
    fe.initMask(MASK_QUARTER);
    fe.loadIntoPixel(0x1);
    fe.initMask(MASK_QUARTER);
    fe.writeRegister(&Fei4::DigHitIn_Sel, 0x1);

    unsigned mask = MASK_QUARTER;
    for (;!(mask&0x8000);mask<<=1) {
        fe.setRunMode(true);
        tx.setTrigEnable(0x1);
        unsigned done = 0;
        unsigned dma_addr = 0;
        unsigned dma_count = 0;
        unsigned rate = 0;
        while (done == 0 && dma_count == 0) {
            dma_count = 0;
            dma_addr = 0;
            rate = spec.readSingle(RX_BRIDGE | RX_DATA_RATE);
            dma_addr = spec.readSingle(RX_BRIDGE | RX_START_ADDR);
            dma_count = spec.readSingle(RX_BRIDGE | RX_DATA_COUNT);
            if (dma_count > 0) {
                uint32_t *buf = new uint32_t[dma_count];
                double readTime = BenchmarkTools::measureReadTime(&spec, dma_addr, buf, dma_count, 1);
                if (readTime < 0) {
                    std::cout << "DMA Failed!" << std::endl;
                    return -1;
                }
                clipRaw.pushData(new RawData(dma_addr, buf, dma_count));
                done = tx.isTrigDone();
            
            }
        }

    }

    Fei4DataProcessor proc;
    proc.connect(&clipRaw, &clipEvent);
        

}
