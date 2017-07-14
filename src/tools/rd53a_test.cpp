#include <iostream>
#include <fstream>

#include "Rd53a.h"
#include "Rd53aEmu.h"
#include "HwController.h"
#include "RingBuffer.h"
#include "EmuController.h"

int main(void) {
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
    myRd53a->wrRegister(6, 6, 6);

    sleep(10);

    emu->run = false;
    emuThreads[0].join();
    delete emu;

//    std::cout << "Test encoding ..." << std::endl;
//    for (uint32_t i=0; i<32; i++)
//        std::cout << "[" << i << "] = " << std::hex << "0x" << Rd53a::encode5to8(i) << std::dec << std::endl;

    return 0;
}
