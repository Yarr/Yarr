// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: RD53A Base class
// # Date: Jun 2017
// ################################

#include "AllChips.h"
#include "Rd53a.h"

bool rd53a_registred =
    StdDict::registerFrontEnd("RD53A", [](){return std::unique_ptr<FrontEnd>(new Rd53a());});

Rd53a::Rd53a() : FrontEnd(), Rd53aCfg(), Rd53aCmd() {
    txChannel = 99;
    rxChannel = 99;
	active = true;
    geo.nRow = 192;
    geo.nCol = 400;
}

Rd53a::Rd53a(TxCore *core) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
    txChannel = 99;
    rxChannel = 99;
	active = true;
    geo.nRow = 192;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

Rd53a::Rd53a(TxCore *core, unsigned arg_channel) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
	txChannel = arg_channel;
	rxChannel = arg_channel;
	active = true;
    geo.nRow = 192;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

Rd53a::Rd53a(TxCore *core, unsigned arg_txChannel, unsigned arg_rxChannel) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
	active = true;
    geo.nRow = 192;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

void Rd53a::init(TxCore *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) {
    this->setCore(arg_core);
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
    geo.nRow = 192;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

void Rd53a::writeRegister(Rd53aReg Rd53aGlobalCfg::*ref, uint32_t value) {
        (this->*ref).write(value);
        wrRegister(m_chipId, (this->*ref).addr(), m_cfg[(this->*ref).addr()]);
}

void Rd53a::readRegister(Rd53aReg Rd53aGlobalCfg::*ref) {
        rdRegister(m_chipId, (this->*ref).addr());
}

void Rd53a::configure() {
    this->configureInit();
    // Turn off clock to matrix
    uint16_t tmp_enCoreColSync = EnCoreColSync.read();
    uint16_t tmp_enCoreColLin1 = EnCoreColLin1.read();
    uint16_t tmp_enCoreColLin2 = EnCoreColLin2.read();
    uint16_t tmp_enCoreColDiff1 = EnCoreColDiff1.read();
    uint16_t tmp_enCoreColDiff2 = EnCoreColDiff2.read();
    EnCoreColSync.write(0);
    EnCoreColLin1.write(0);
    EnCoreColLin2.write(0);
    EnCoreColDiff1.write(0);
    EnCoreColDiff2.write(0);
    // Write globals
    this->configureGlobal();
    while(!core->isCmdEmpty()){;}
    // Write pixels
    this->configurePixels();
    while(!core->isCmdEmpty()){;}
    // Turn on clock to matrix
    this->writeRegister(&Rd53a::EnCoreColSync, tmp_enCoreColSync);
    this->writeRegister(&Rd53a::EnCoreColLin1, tmp_enCoreColLin1);
    this->writeRegister(&Rd53a::EnCoreColLin2, tmp_enCoreColLin2);
    this->writeRegister(&Rd53a::EnCoreColDiff1, tmp_enCoreColDiff1);
    this->writeRegister(&Rd53a::EnCoreColDiff2, tmp_enCoreColDiff2);
    while(!core->isCmdEmpty()){;}
}

void Rd53a::configureInit() {
    this->writeRegister(&Rd53a::GlobalPulseRt, 0x007F); // Reset a whole bunch of things
    this->globalPulse(m_chipId, 8);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    this->writeRegister(&Rd53a::GlobalPulseRt, 0x4100); //activate monitor and prime sync FE AZ
    this->globalPulse(m_chipId, 8);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    this->ecr();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    this->bcr();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    while(!core->isCmdEmpty()){;}
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
    this->writeRegister(&Rd53a::PixAutoCol, 0);
    this->writeRegister(&Rd53a::PixAutoRow, 0);

    // Writing two columns and six rows at the same time
    for (unsigned col=0; col<n_Col; col+=2) {
        this->writeRegister(&Rd53a::PixRegionCol, col/2);
        //this->writeRegister(&Rd53a::PixRegionRow, 0); 
        for (unsigned row=0; row<n_Row; row+=1) {
            this->writeRegister(&Rd53a::PixRegionRow, row); 
            //this->wrRegisterBlock(m_chipId, 0, &pixRegs[Rd53aPixelCfg::toIndex(col, row)]);
            this->writeRegister(&Rd53a::PixPortal, pixRegs[Rd53aPixelCfg::toIndex(col, row)]);
            //if (pixRegs[Rd53aPixelCfg::toIndex(col, row)] != 0x0)
            //    std::cout << "[" << col << "][" << row << "] = 0x" << std::hex << pixRegs[Rd53aPixelCfg::toIndex(col, row)] << std::dec << std::endl;
            if (row % 20 == 0)
                while(!core->isCmdEmpty()){;}
        }
    }
}

void Rd53a::configurePixels(std::vector<std::pair<unsigned, unsigned>> &pixels) {
    // Setup pixel programming
    this->writeRegister(&Rd53a::PixAutoCol, 0);
    this->writeRegister(&Rd53a::PixAutoRow, 0);
    int counter = 0;
    int old_col = -1;
    //std::cout << "Seeing " << pixels.size() << " modified pixels!" << std::endl;
    for (auto &pixel : pixels) {
        if (old_col != (int)pixel.first/2) {
            this->writeRegister(&Rd53a::PixRegionCol, pixel.first/2);
            old_col = pixel.first/2;
        }
        this->writeRegister(&Rd53a::PixRegionRow, pixel.second); 
        this->writeRegister(&Rd53a::PixPortal, pixRegs[Rd53aPixelCfg::toIndex(pixel.first, pixel.second)]);
        counter++;
        if (counter == 20 ) {
            while(!core->isCmdEmpty()){;}
            counter = 0;
        }
    }
}

void Rd53a::writeNamedRegister(std::string name, uint16_t value) {
    std::cout << __PRETTY_FUNCTION__ << " : " << name << " -> " << value << std::endl;
    if (regMap.find(name) != regMap.end())
        writeRegister(regMap[name], value);
}

// TODO remove magic numbers
// Move to config part
void Rd53a::enableCalCol(unsigned col) {
    if (col < 16) {
        this->writeRegister(&Rd53a::CalColprSync1, CalColprSync1.read() | (0x1 << col));
    } else if (col < 32) {
        this->writeRegister(&Rd53a::CalColprSync2, CalColprSync2.read() | (0x1 << (col-16)));
    } else if (col < 48) {
        this->writeRegister(&Rd53a::CalColprSync3, CalColprSync3.read() | (0x1 << (col-32)));
    } else if (col < 64) {
        this->writeRegister(&Rd53a::CalColprSync4, CalColprSync4.read() | (0x1 << (col-48)));
    } else if (col < 80) {
        this->writeRegister(&Rd53a::CalColprLin1, CalColprLin1.read() | (0x1 << (col-64)));
    } else if (col < 96) {
        this->writeRegister(&Rd53a::CalColprLin2, CalColprLin2.read() | (0x1 << (col-80)));
    } else if (col < 112) {
        this->writeRegister(&Rd53a::CalColprLin3, CalColprLin3.read() | (0x1 << (col-96)));
    } else if (col < 128) {
        this->writeRegister(&Rd53a::CalColprLin4, CalColprLin4.read() | (0x1 << (col-112)));
    } else if (col < 132) {
        this->writeRegister(&Rd53a::CalColprLin5, CalColprLin5.read() | (0x1 << (col-128)));
    } else if (col < 148) {
        this->writeRegister(&Rd53a::CalColprDiff1, CalColprDiff1.read() | (0x1 << (col-132)));
    } else if (col < 164) {
        this->writeRegister(&Rd53a::CalColprDiff2, CalColprDiff2.read() | (0x1 << (col-148)));
    } else if (col < 180) {
        this->writeRegister(&Rd53a::CalColprDiff3, CalColprDiff3.read() | (0x1 << (col-164)));
    } else if (col < 196) {
        this->writeRegister(&Rd53a::CalColprDiff4, CalColprDiff4.read() | (0x1 << (col-180)));
    } else if (col < 200) {
        this->writeRegister(&Rd53a::CalColprDiff5, CalColprDiff5.read() | (0x1 << (col-196)));
    } else {
        std::cout << __PRETTY_FUNCTION__ << " --> col (" << col << ") out of range!" << std::endl;
    }
}

void Rd53a::disableCalCol(unsigned col) {
    if (col < 16) {
        this->writeRegister(&Rd53a::CalColprSync1, CalColprSync1.read()  & ~(0x1 << col));
    } else if (col < 32) {
        this->writeRegister(&Rd53a::CalColprSync2, CalColprSync2.read() & ~(0x1 << (col-16)));
    } else if (col < 48) {
        this->writeRegister(&Rd53a::CalColprSync3, CalColprSync3.read() & ~(0x1 << (col-32)));
    } else if (col < 64) {
        this->writeRegister(&Rd53a::CalColprSync4, CalColprSync4.read() & ~(0x1 << (col-48)));
    } else if (col < 80) {
        this->writeRegister(&Rd53a::CalColprLin1, CalColprLin1.read() & ~(0x1 << (col-64)));
    } else if (col < 96) {
        this->writeRegister(&Rd53a::CalColprLin2, CalColprLin2.read() & ~(0x1 << (col-80)));
    } else if (col < 112) {
        this->writeRegister(&Rd53a::CalColprLin3, CalColprLin3.read() & ~(0x1 << (col-96)));
    } else if (col < 128) {
        this->writeRegister(&Rd53a::CalColprLin4, CalColprLin4.read() & ~(0x1 << (col-112)));
    } else if (col < 132) {
        this->writeRegister(&Rd53a::CalColprLin5, CalColprLin5.read() & ~(0x1 << (col-128)));
    } else if (col < 148) {
        this->writeRegister(&Rd53a::CalColprDiff1, CalColprDiff1.read() & ~(0x1 << (col-132)));
    } else if (col < 164) {
        this->writeRegister(&Rd53a::CalColprDiff2, CalColprDiff2.read() & ~(0x1 << (col-148)));
    } else if (col < 180) {
        this->writeRegister(&Rd53a::CalColprDiff3, CalColprDiff3.read() & ~(0x1 << (col-164)));
    } else if (col < 196) {
        this->writeRegister(&Rd53a::CalColprDiff4, CalColprDiff4.read() & ~(0x1 << (col-180)));
    } else if (col < 200) {
        this->writeRegister(&Rd53a::CalColprDiff5, CalColprDiff5.read() & ~(0x1 << (col-196)));
    } else {
        std::cout << __PRETTY_FUNCTION__ << " --> col (" << col << ") out of range!" << std::endl;
    }
}


