#include <iostream>
#include <iomanip>
#include <fstream>

#include "SpecController.h"
#include "TxCore.h"
#include "RxCore.h"
#include "Fe65p2.h"
#include "Histo2d.h"

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    
    if (argc != 2) {
        std::cout << "Error no config file" << std::endl;
        return -1;
    }

    SpecController spec(0);
    TxCore tx(&spec);
    RxCore rx(&spec);

    Fe65p2 *fe = new Fe65p2(&tx);
    Fe65p2 *g_fe = new Fe65p2(&tx);

    std::fstream cfgFile(argv[1], std::ios::in);
    json cfg;
    cfgFile >> cfg;

    cfgFile.close();

    // Load config
    fe->fromFileJson(cfg);
    g_fe->fromFileJson(cfg);

    // Enable link
    tx.setCmdEnable(0x1);
    tx.toggleTrigAbort();
    // Configure FE
    g_fe->configure();

    g_fe->enAnaInj();
    g_fe->setPlsrDac(1000);
    tx.setTrigEnable(0x0);

    while(!tx.isCmdEmpty());
    
    int num_inject = 50;

    Histo2d enMask("enMask", 64, 0.5, 64.5, 64, 0.5, 64.5, typeid(void));
    for (unsigned i=0; i<16; i++) {
        uint32_t colEn = 0x1 << i;
        g_fe->setValue(&Fe65p2::ColEn, colEn);
        g_fe->configureGlobal();
        while(!tx.isCmdEmpty());
        for(unsigned n=0; n<16; n++) {
            g_fe->PixConf(n).setAll(0);
            g_fe->InjEn(n).setAll(0);
            while(!tx.isCmdEmpty());
        }
        for (unsigned j=0; j<256; j++) {
            if (cfg["FE65-P2"]["PixelConfig"][(i*4)+(j/64)]["PixConf"][j%64] == 0)
                continue;
            g_fe->PixConf(i).setAll(0);
            g_fe->InjEn(i).setAll(0);
            g_fe->setPixConf((i*4)+(j/64)+1, (j%64)+1, 3);
            g_fe->setInjEn((i*4)+(j/64)+1, (j%64)+1, 1);
            g_fe->configurePixels();
            while(!tx.isCmdEmpty());
            
            tx.setTrigConfig(INT_COUNT);
            tx.setTrigCnt(num_inject);
            tx.setTrigFreq(1.0e3);
            uint32_t trigWord[4];
            trigWord[0] = 0x00;
            trigWord[1] = 0x00;
            trigWord[2] = 0x00;
            trigWord[3] = MOJO_HEADER + (PULSE_REG << 16) + PULSE_INJECT;
            tx.setTrigWord(trigWord);

            tx.setTrigEnable(0x1);
            while(!tx.isTrigDone());
            tx.setTrigEnable(0x0);
            
            int num_hitbus = tx.getTrigInCount();
            if (num_hitbus < (2*num_inject)*0.8 || num_hitbus > (2*num_inject)*1.2) {
                std::cout << "[" << (i*4)+(j/64)+1 << "][" << (j%64)+1 << "] = " << num_hitbus << std::endl;
                fe->setPixConf((i*4)+(j/64)+1, (j%64)+1, 2); // => HitEn = 1, HitOrEn = 0
		enMask.setBin(enMask.binNum((i*4)+(j/64)+1, (j%64)+1), 0);
            } else {
		enMask.setBin(enMask.binNum((i*4)+(j/64)+1, (j%64)+1), 1);
            }
        }
    }

    fe->toFileJson(cfg);
    std::fstream outFile(("masked_" + std::string(argv[1])).c_str(), std::ios::out);
    outFile << std::setw(4) << cfg;
    outFile.close();

    return 0;
}
