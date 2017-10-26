#include <iostream>
#include <fstream>

#include "Rd53a.h"
#include "Rd53aEmu.h"
#include "HwController.h"
#include "RingBuffer.h"
#include "EmuController.h"

int main(void) {
    Rd53aCfg *feCfg = new Rd53aCfg();

    HwController *hwCtrl = NULL;
    Rd53aEmu *emu = NULL;

    RingBuffer * rx = new RingBuffer(128);
    RingBuffer * tx = new RingBuffer(128);

    std::string ctrlCfgPath = "configs/emuCfg.json";
    std::ifstream ctrlCfgFile(ctrlCfgPath);
    json ctrlCfg;
    ctrlCfg << ctrlCfgFile;

    hwCtrl = new EmuController(rx, tx);
    hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);

    std::vector<std::thread> emuThreads;
    emu= new Rd53aEmu(rx, tx);
    emuThreads.push_back(std::thread(&Rd53aEmu::executeLoop, emu));

    Rd53a* myRd53a = new Rd53a((TxCore*) hwCtrl);

/*
    // these commands will write the global configuration
    myRd53a->wrRegister(4, 1, (uint32_t) 0);
    myRd53a->wrRegister(4, 2, (uint32_t) 0);
    myRd53a->wrRegister(4, 3, (uint32_t) 0);
    myRd53a->wrRegister(4, 4, 0x9ce2);
    myRd53a->wrRegister(4, 5, (uint32_t) 100);
    myRd53a->wrRegister(4, 6, (uint32_t) 150);
    myRd53a->wrRegister(4, 7, (uint32_t) 100);
    myRd53a->wrRegister(4, 8, (uint32_t) 140);
    myRd53a->wrRegister(4, 9, (uint32_t) 200);
    myRd53a->wrRegister(4, 10, (uint32_t) 100);
    myRd53a->wrRegister(4, 11, (uint32_t) 450);
    myRd53a->wrRegister(4, 12, (uint32_t) 300);
    myRd53a->wrRegister(4, 13, (uint32_t) 490);
    myRd53a->wrRegister(4, 14, (uint32_t) 300);
    myRd53a->wrRegister(4, 15, (uint32_t) 20);
    myRd53a->wrRegister(4, 16, (uint32_t) 50);
    myRd53a->wrRegister(4, 17, (uint32_t) 80);
    myRd53a->wrRegister(4, 18, (uint32_t) 110);
    myRd53a->wrRegister(4, 19, (uint32_t) 300);
    myRd53a->wrRegister(4, 20, (uint32_t) 408);
    myRd53a->wrRegister(4, 21, (uint32_t) 533);
    myRd53a->wrRegister(4, 22, (uint32_t) 542);
    myRd53a->wrRegister(4, 23, (uint32_t) 551);
    myRd53a->wrRegister(4, 24, (uint32_t) 528);
    myRd53a->wrRegister(4, 25, (uint32_t) 164);
    myRd53a->wrRegister(4, 26, (uint32_t) 1023);
    myRd53a->wrRegister(4, 27, (uint32_t) 0);
    myRd53a->wrRegister(4, 28, (uint32_t) 20);
    myRd53a->wrRegister(4, 29, (uint32_t) (1 << 1));
    myRd53a->wrRegister(4, 30, (uint32_t) (1 << 2));
    myRd53a->wrRegister(4, 31, (uint32_t) (16 << 5) + 16);
    myRd53a->wrRegister(4, 32, (uint32_t) 0xffff);
    myRd53a->wrRegister(4, 33, (uint32_t) 0xffff);
    myRd53a->wrRegister(4, 34, (uint32_t) 1);
    myRd53a->wrRegister(4, 35, (uint32_t) 0xff);
    myRd53a->wrRegister(4, 36, (uint32_t) 1);
    myRd53a->wrRegister(4, 37, (uint32_t) 500);
    myRd53a->wrRegister(4, 38, (uint32_t) 16);
    myRd53a->wrRegister(4, 39, (uint32_t) (1 << 4));
    myRd53a->wrRegister(4, 40, (uint32_t) 0);
    myRd53a->wrRegister(4, 41, (uint32_t) 500);
    myRd53a->wrRegister(4, 42, (uint32_t) 300);
    myRd53a->wrRegister(4, 43, (uint32_t) 0 + (16 << 4) + 8);
    myRd53a->wrRegister(4, 44, (uint32_t) 0);
    myRd53a->wrRegister(4, 45, (uint32_t) 0);
    myRd53a->wrRegister(4, 46, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 47, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 48, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 49, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 50, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 51, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 52, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 53, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 54, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 55, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 56, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 57, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 58, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 59, (uint32_t) 0xf);
    myRd53a->wrRegister(4, 60, (uint32_t) 0);
    myRd53a->wrRegister(4, 61, (uint32_t) (1 << 2));
    myRd53a->wrRegister(4, 62, (uint32_t) 0);
    myRd53a->wrRegister(4, 63, (uint32_t) 0);
    myRd53a->wrRegister(4, 64, (uint32_t) 0);
    myRd53a->wrRegister(4, 65, (uint32_t) 400);
    myRd53a->wrRegister(4, 66, (uint32_t) 50);
    myRd53a->wrRegister(4, 67, (uint32_t) 500);
    myRd53a->wrRegister(4, 68, (uint32_t) (1 << 6) + (1 << 4) + (1 << 2) + 1);
    myRd53a->wrRegister(4, 69, (uint32_t) 0 + (3 << 4) + 0xf);
    myRd53a->wrRegister(4, 70, (uint32_t) 500);
    myRd53a->wrRegister(4, 71, (uint32_t) 0);
    myRd53a->wrRegister(4, 72, (uint32_t) 0);
    myRd53a->wrRegister(4, 73, (uint32_t) (25 << 2) + 3);
    myRd53a->wrRegister(4, 74, (uint32_t) (15 << 4));
    myRd53a->wrRegister(4, 75, (uint32_t) 15);
    myRd53a->wrRegister(4, 76, (uint32_t) 32);
    myRd53a->wrRegister(4, 77, (uint32_t) 0 + (63 << 7) + 127);
    myRd53a->wrRegister(4, 78, (uint32_t) 0);
    myRd53a->wrRegister(4, 79, (uint32_t) 0);
    myRd53a->wrRegister(4, 80, (uint32_t) 0);
    myRd53a->wrRegister(4, 81, (uint32_t) 0);
    myRd53a->wrRegister(4, 82, (uint32_t) 0);
    myRd53a->wrRegister(4, 83, (uint32_t) 0);
    myRd53a->wrRegister(4, 84, (uint32_t) 0);
    myRd53a->wrRegister(4, 85, (uint32_t) 0);
    myRd53a->wrRegister(4, 86, (uint32_t) 0);
    myRd53a->wrRegister(4, 87, (uint32_t) 0);
    myRd53a->wrRegister(4, 88, (uint32_t) 0);
    myRd53a->wrRegister(4, 89, (uint32_t) 0);
    myRd53a->wrRegister(4, 90, (uint32_t) 0);
    myRd53a->wrRegister(4, 91, (uint32_t) 0);
    myRd53a->wrRegister(4, 92, (uint32_t) 0);
    myRd53a->wrRegister(4, 93, (uint32_t) 0);
    myRd53a->wrRegister(4, 94, (uint32_t) 0);
    myRd53a->wrRegister(4, 95, (uint32_t) 0);
    myRd53a->wrRegister(4, 96, (uint32_t) 0);
    myRd53a->wrRegister(4, 97, (uint32_t) 0);
    myRd53a->wrRegister(4, 98, (uint32_t) 0);
    myRd53a->wrRegister(4, 99, (uint32_t) 0);
    myRd53a->wrRegister(4, 100, (uint32_t) 0);
    myRd53a->wrRegister(4, 101, (uint32_t) 136);
    myRd53a->wrRegister(4, 102, (uint32_t) 130);
    myRd53a->wrRegister(4, 103, (uint32_t) 118);
    myRd53a->wrRegister(4, 104, (uint32_t) 119);
    myRd53a->wrRegister(4, 105, (uint32_t) 120);
    myRd53a->wrRegister(4, 106, (uint32_t) 121);
    myRd53a->wrRegister(4, 107, (uint32_t) 122);
    myRd53a->wrRegister(4, 108, (uint32_t) 123);
    myRd53a->wrRegister(4, 109, (uint32_t) 0);
    myRd53a->wrRegister(4, 110, (uint32_t) 0);
    myRd53a->wrRegister(4, 111, (uint32_t) 0);
    myRd53a->wrRegister(4, 112, (uint32_t) 0);
    myRd53a->wrRegister(4, 113, (uint32_t) 0);
    myRd53a->wrRegister(4, 114, (uint32_t) 0);
    myRd53a->wrRegister(4, 115, (uint32_t) 0);
    myRd53a->wrRegister(4, 116, (uint32_t) 0);
    myRd53a->wrRegister(4, 117, (uint32_t) 0);
    myRd53a->wrRegister(4, 118, (uint32_t) 0);
    myRd53a->wrRegister(4, 119, (uint32_t) 0);
    myRd53a->wrRegister(4, 120, (uint32_t) 0);
    myRd53a->wrRegister(4, 121, (uint32_t) 0);
    myRd53a->wrRegister(4, 122, (uint32_t) 0);
    myRd53a->wrRegister(4, 123, (uint32_t) 0);
    myRd53a->wrRegister(4, 124, (uint32_t) 0);
    myRd53a->wrRegister(4, 125, (uint32_t) 0);
    myRd53a->wrRegister(4, 126, (uint32_t) 0);
    myRd53a->wrRegister(4, 127, (uint32_t) 0);
    myRd53a->wrRegister(4, 128, (uint32_t) 0);
    myRd53a->wrRegister(4, 129, (uint32_t) 0);
    myRd53a->wrRegister(4, 130, (uint32_t) 0);
    myRd53a->wrRegister(4, 131, (uint32_t) 0);
    myRd53a->wrRegister(4, 132, (uint32_t) 0);
    myRd53a->wrRegister(4, 133, (uint32_t) 0);
    myRd53a->wrRegister(4, 134, (uint32_t) 0);
    myRd53a->wrRegister(4, 135, (uint32_t) 0);
    myRd53a->wrRegister(4, 136, (uint32_t) 0);
    myRd53a->wrRegister(4, 137, (uint32_t) 0);
*/
    // set some value for pixel registers
    feCfg->PixPortalHigh.write(0xFF);
    feCfg->PixPortalLow.write(0xFF);

    // test retrieving registers by their name
    uint32_t test = feCfg->getValue(&Rd53aGlobalCfg::PixPortalHigh);
    printf("test = 0x%x\n", test);

    // use the new write register function to test
    myRd53a->writeRegister(4, &Rd53aGlobalCfg::PixPortalHigh, 0xAA);
    myRd53a->writeRegister(4, &Rd53aGlobalCfg::RegionCol, 0xFF);

    // these commands should register all pixel registers
    myRd53a->wrRegister(4, 0, (uint32_t) 0xFF);
    myRd53a->wrRegister(4, 1, (uint32_t) 0xFF); // 1 = RegionCol
    myRd53a->wrRegister(4, 2, (uint32_t) 0); // 2 = RegionRow
    myRd53a->wrRegister(4, 3, 0x00000027); // 3 = PixMode + BMask; 0x17 = 0x100111 bits = auto col, auto row, broadcast, fe flavor 1, fe flavor 2, fe flavor 3;
    for (int i = 0; i < 192; i++) { // since se set auto col to 1, but not auto row, the chip/emulator will loop over all columns, but we must here manually loop over all rows
        myRd53a->wrRegister(4, 0, (feCfg->PixPortalHigh.read() << 8) + feCfg->PixPortalLow.read()); // do we actually need to send the data here? - I think yes
    }

    // try to mask some columns
//    myRd53a->writeRegister(4, &Rd53aGlobalCfg::EnCoreColSync, 0x0000);
    myRd53a->writeRegister(4, &Rd53aGlobalCfg::CalColprDiff3, 0x0000);

    // send a trigger - this could be put into a loop with various ways of masking
    myRd53a->trigger(0x2B, 0);

    sleep(2);

    emu->run = false;
    emuThreads[0].join();
    delete emu;

//    std::cout << "Test encoding ..." << std::endl;
//    for (uint32_t i=0; i<32; i++)
//        std::cout << "[" << i << "] = " << std::hex << "0x" << Rd53a::encode5to8(i) << std::dec << std::endl;

    return 0;
}
