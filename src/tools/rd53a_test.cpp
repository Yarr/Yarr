#include <iostream>
#include <chrono>
#include <unistd.h>
#include "SpecController.h"
#include "Rd53a.h"

#define EN_RX2 0x1
#define EN_RX1 0x2
#define EN_RX4 0x4
#define EN_RX3 0x8
#define EN_RX6 0x10
#define EN_RX5 0x20
#define EN_RX8 0x40
#define EN_RX7 0x80

#define EN_RX10 0x100
#define EN_RX9 0x200
#define EN_RX12 0x400
#define EN_RX11 0x800
#define EN_RX14 0x1000
#define EN_RX13 0x2000
#define EN_RX16 0x4000
#define EN_RX15 0x8000

#define EN_RX18 0x10000
#define EN_RX17 0x20000
#define EN_RX20 0x40000
#define EN_RX19 0x80000
#define EN_RX22 0x100000
#define EN_RX21 0x200000
#define EN_RX24 0x400000
#define EN_RX23 0x800000

void decode(RawData *data) {
    if (data != NULL) {
        for (unsigned i=0; i<data->words; i++) {
            if (data->buf[i] != 0xFFFFFFFF) {
                if ((data->buf[i] >> 25) & 0x1) {
                    unsigned l1id = 0x1F & (data->buf[i] >> 20);
                    unsigned l1tag = 0x1F & (data->buf[i] >> 15);
                    unsigned bcid = 0x7FFF & data->buf[i];
                    std::cout << "[Header] : L1ID(" << l1id << ") L1Tag(" << l1tag << ") BCID(" <<  bcid << ")" << std::endl;
                } else {
                    unsigned core_col = 0x3F & (data->buf[i] >> 26);
                    unsigned core_row = 0x3F & (data->buf[i] >> 17);
                    unsigned parity = 0x1 & (data->buf[i] >> 16);
                    unsigned tot0 = 0xF & (data->buf[i] >> 0);
                    unsigned tot1 = 0xF & (data->buf[i] >> 4);
                    unsigned tot2 = 0xF & (data->buf[i] >> 8);
                    unsigned tot3 = 0xF & (data->buf[i] >> 12);
                    std::cout << "[Data] : COL(" << core_col << ") ROW(" << core_row << ") PAR(" << parity
                        << ") TOT(" << tot3 << "," << tot2 << "," << tot1 << "," << tot0 << ")" << std::endl;
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {

    int specNum = 0;
    if (argc > 1)
        specNum = std::stoi(argv[1]);

    SpecController spec;
    spec.init(specNum);
    spec.toggleTrigAbort();
    spec.setTrigEnable(0);
    
    //Send IO config to active FMC
    //spec.writeSingle(0x6<<14 | 0x0, EN_RX1 | EN_RX3 | EN_RX4 | EN_RX5);
    //spec.writeSingle(0x6<<14 | 0x0, 0xFFFFFFE);
    spec.writeSingle(0x6<<14 | 0x0, 0x080000);
    spec.writeSingle(0x6<<14 | 0x1, 0xF);
    spec.setCmdEnable(0x1);
    spec.setRxEnable(0x0);
    
    Rd53a fe(&spec);
    fe.setChipId(0);
    for (unsigned col=0; col<Rd53a::n_Col; col++) {
        for(unsigned row=0; row<Rd53a::n_Row; row++) {
            fe.setEn(col, row, 0); // Disable all to avoid noise
        }
    }
    std::cout << ">>> Configuring chip with default config ..." << std::endl;
    fe.configure();
    std::cout << " ... done." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // TODO check link sync
    
    uint32_t linkStatus = spec.readSingle(RX_ADDR | RX_STATUS);
    std::cout << ">>> Checking link status: 0x" << std::hex << linkStatus << std::dec << std::endl;
    if (linkStatus > 0x0) {
        std::cout << "   All links are synced!" << std::endl;
    } else {
        std::cerr << "   #ERROR# Not all 4 lanes are synced! Aborting!" << std::endl;
        return 0;
    }
    
    spec.setRxEnable(0x1);

    std::cout << ">>> Trigger test:" << std::endl;
    for (unsigned i=1; i<16; i++) {
        std::cout << "Trigger: " << i << std::endl;
        fe.trigger(i, i);
        RawData *data = NULL;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (data != NULL)
                delete data;
            data = spec.readData();
            decode(data);
        } while (data != NULL);
    }
    
    std::cout << ">>> Enabling digital injection" << std::endl;
    fe.writeRegister(&Rd53a::InjEnDig, 1);
    fe.writeRegister(&Rd53a::LatencyConfig, 48);
    fe.writeRegister(&Rd53a::GlobalPulseRt, 16384);
    fe.writeRegister(&Rd53a::EnCoreColSync, 0x0);
   
    std::cout << ">>> Enabling some pixels" << std::endl;
    
    fe.setEn(150, 0, 1);
    fe.setInjEn(150, 0, 1);
    fe.setEn(165, 0, 1);
    fe.setInjEn(165, 0, 1);
    fe.setEn(165, 1, 1);
    fe.setInjEn(165, 1, 1);

    /*
    fe.writeRegister(&Rd53a::PixRegionCol, 100);
    fe.writeRegister(&Rd53a::PixRegionRow, 0);
    fe.writeRegister(&Rd53a::PixPortal, 0xFFFF);
    fe.writeRegister(&Rd53a::PixRegionRow, 1);
    fe.writeRegister(&Rd53a::PixPortal, 0xFFFF);
    fe.writeRegister(&Rd53a::PixRegionRow, 2);
    fe.writeRegister(&Rd53a::PixPortal, 0xFFFF);
    */
    fe.configurePixels();
    while (!spec.isCmdEmpty()) {}

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    std::cout << ">>> Digital inject test:" << std::endl;
    
    // {Cal,Cal}{ChipId[3:0],CalEdgeMode,CalEdgeDelay[2:0],CalEdgeWidth[5:4]}{CalEdgeWidth[3:0],CalAuxMode,CalAuxDly[4:0]}
    //void Rd53aCmd::cal(uint32_t chipId, uint32_t mode, uint32_t delay, uint32_t duration, uint32_t aux_mode, uint32_t aux_delay) {
    /*
    fe.cal(0, 1, 0, 50, 0, 0);
    spec.writeFifo(0x69696969); // Two idles = 8 BCs
    spec.writeFifo(0x69696969); // 16
    spec.writeFifo(0x69696969); // 24
    spec.writeFifo(0x69696969); // 32
    fe.trigger(0xF, 31, 0xF, 2);
    */
    
    spec.setTrigFreq(5000);
    spec.setTrigCnt(1);
    spec.setTrigWordLength(16);
    std::array<uint32_t, 32> trigWord;
    trigWord.fill(0x69696969);
    trigWord[15] = 0x69696363;
    trigWord[14] = Rd53aCmd::genCal(8, 1, 0, 10, 0, 0); // Inject
    trigWord[8] = Rd53aCmd::genTrigger(0xF, 1, 0xF, 2); // Trigger
    //trigWord[7] = Rd53aCmd::genTrigger(0xF, 3, 0xF, 4); // Trigger
    //trigWord[2] = 0x5c5c0000 + (Rd53aCmd::encode5to8(0x8<<1)<<8) + (Rd53aCmd::encode5to8(8<<1)); // global pulse for sync FE
    //trigWord[1] = 0x5a5a6363; // ECR + header
    //trigWord[0] = Rd53aCmd::genCal(8, 1, 0, 0, 0, 0); // Arm inject
    spec.setTrigWord(&trigWord[0], 32);
    spec.setTrigConfig(INT_COUNT);
    spec.setTrigEnable(1);
    while(!spec.isTrigDone()) {
        RawData *data = NULL;
        do {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (data != NULL)
                delete data;
            data = spec.readData();
            decode(data);
        } while (data != NULL);

    }
    spec.setTrigEnable(0);
    spec.setRxEnable(0x0);
    return 0;
}
