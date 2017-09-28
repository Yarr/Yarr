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
//    myRd53a->wrRegister(4, 6, 6);

    // set some value for pixel registers
    feCfg->PixPortalHigh.write(0xFF);
    feCfg->PixPortalLow.write(0xFF);

    // these commands should register all pixel registers
    myRd53a->wrRegister(0, 0, (uint32_t) 0xFF);
    myRd53a->wrRegister(4, 1, (uint32_t) 0xFF); // 1 = RegionCol
    myRd53a->wrRegister(4, 2, (uint32_t) 2); // 2 = RegionRow
    myRd53a->wrRegister(4, 3, 0x00000017); // 3 = PixMode + BMask; 0x17 = 0x010111 bits = auto col, auto row, broadcast, fe flavor 1, fe flavor 2, fe flavor 3;
    for (int i = 0; i < 1; i++) { // make this loop to i < 193 in order to loop over all rows
        myRd53a->wrRegister(4, 0, (feCfg->PixPortalHigh.read() << 8) + feCfg->PixPortalLow.read()); // do we actually need to send the data here?
    }

    sleep(10);

    emu->run = false;
    emuThreads[0].join();
    delete emu;

//    std::cout << "Test encoding ..." << std::endl;
//    for (uint32_t i=0; i<32; i++)
//        std::cout << "[" << i << "] = " << std::hex << "0x" << Rd53a::encode5to8(i) << std::dec << std::endl;

    return 0;
}
