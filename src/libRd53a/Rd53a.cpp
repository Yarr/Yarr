// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: RD53A Base class
// # Date: Jun 2017
// ################################

#include "Rd53a.h"

Rd53a::Rd53a(TxCore *core) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
    txChannel = 99;
    rxChannel = 99;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

Rd53a::Rd53a(TxCore *core, unsigned arg_channel) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
	txChannel = arg_channel;
	rxChannel = arg_channel;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

Rd53a::Rd53a(TxCore *core, unsigned arg_txChannel, unsigned arg_rxChannel) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
	histogrammer = NULL;
	ana = NULL;
	active = true;
}

void Rd53a::writeRegister(Rd53aReg Rd53aGlobalCfg::*ref, uint32_t value) {
        (this->*ref).write(value);
        wrRegister(m_chipId, (this->*ref).addr(), m_cfg[(this->*ref).addr()]);
}

void Rd53a::configure() {
    this->init();
    this->configureGlobal();
    while(!core->isCmdEmpty()){;}
    this->configurePixels();
    while(!core->isCmdEmpty()){;}
}

void Rd53a::init() {
    this->ecr();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    this->bcr();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    this->writeRegister(&Rd53a::GlobalPulseRt, 0x17F); // Reset a whole bunch of things
    this->globalPulse(m_chipId, 10);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    this->writeRegister(&Rd53a::GlobalPulseRt, 0x0);
}

void Rd53a::configureGlobal() {
    for (unsigned addr=0; addr<numRegs; addr++) {
        this->wrRegister(m_chipId, addr, m_cfg[addr]);
        if (addr % 20 == 0)
            while(!core->isCmdEmpty()){;}
    }
}

void Rd53a::configurePixels() {
    // Setup pixel programming
    this->writeRegister(&Rd53a::PixRegionCol, 0); 
    this->writeRegister(&Rd53a::PixRegionRow, 0); 
    this->writeRegister(&Rd53a::PixAutoCol, 1);
    this->writeRegister(&Rd53a::PixAutoRow, 1);

    // Writing two columns and six rows at the same time
    for (unsigned col=0; col<n_Col; col+=2) {
        for (unsigned row=0; row<n_Row; row+=6) {
            this->wrRegisterBlock(m_chipId, 0, &pixRegs[Rd53aPixelCfg::toIndex(col, row)]);
            if (row % 24 == 0)
                while(!core->isCmdEmpty()){;}
        }
    }
}
