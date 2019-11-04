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
#include "RawData.h"

bool rd53a_registred =
    StdDict::registerFrontEnd("RD53A", [](){return std::unique_ptr<FrontEnd>(new Rd53a());});

Rd53a::Rd53a() : FrontEnd(), Rd53aCfg(), Rd53aCmd() {
    txChannel = 99;
    rxChannel = 99;
	active = true;
    geo.nRow = 192;
    geo.nCol = 400;
}

Rd53a::Rd53a(HwController *core) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
    m_rxcore = core;
    txChannel = 99;
    rxChannel = 99;
	active = true;
    geo.nRow = 192;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

Rd53a::Rd53a(HwController *core, unsigned arg_channel) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
    m_rxcore = core;
	txChannel = arg_channel;
	rxChannel = arg_channel;
	active = true;
    geo.nRow = 192;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

Rd53a::Rd53a(HwController *core, unsigned arg_txChannel, unsigned arg_rxChannel) : FrontEnd(), Rd53aCfg(), Rd53aCmd(core) {
    m_rxcore = core;
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
	active = true;
    geo.nRow = 192;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

void Rd53a::init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) {
    this->setCore(arg_core);
    m_rxcore = arg_core;
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




//This only works for voltage MUX values. 
void Rd53a::confADC(uint16_t MONMUX,bool doCur=false) {

  uint16_t OriginalGlobalRT = this->GlobalPulseRt.read();

  if(doCur)
    this->writeRegister(&Rd53a::MonitorImonMux,  MONMUX); //Select what to monitor
  else
    this->writeRegister(&Rd53a::MonitorVmonMux,  MONMUX); //Select what to monitor    


  this->writeRegister(&Rd53a::MonitorEnable, 1); //Enabling monitoring 

  this->writeRegister(&Rd53a::GlobalPulseRt ,64); //ResetADC
  this->idle();
  this->globalPulse(m_chipId, 4);  
  std::this_thread::sleep_for(std::chrono::microseconds(100)); 

  this->writeRegister(&Rd53a::GlobalPulseRt ,4096); //Trigger ADC Conversion
  this->idle();
  this->globalPulse(m_chipId, 4);  

  std::this_thread::sleep_for(std::chrono::microseconds(1000)); //This is neccessary to clean. This might be controller dependent.  

  this->writeRegister(&Rd53a::GlobalPulseRt ,OriginalGlobalRT); //Trigger ADC Conversion
}


void Rd53a::RunRingOsc(uint16_t Durration) {
  uint16_t OriginalGlobalRT = this->GlobalPulseRt.read();

  this->writeRegister(&Rd53a::GlobalPulseRt ,0x2000); //ResetADC
  this->idle();
  this->globalPulse(m_chipId, Durration);  

  std::this_thread::sleep_for(std::chrono::microseconds(1000)); //This is neccessary to clean. This might be controller dependent.  

  this->writeRegister(&Rd53a::GlobalPulseRt ,OriginalGlobalRT); //Trigger ADC Conversion



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
    this->writeRegister(&Rd53a::PixAutoRow, 1);

    // Writing two columns and six rows at the same time
    for (unsigned col=0; col<n_Col; col+=2) {
        this->writeRegister(&Rd53a::PixRegionCol, col/2);
        this->writeRegister(&Rd53a::PixRegionRow, 0); 
        for (unsigned row=0; row<n_Row; row+=1) {
            //this->writeRegister(&Rd53a::PixRegionRow, row); 
            //this->wrRegisterBlock(m_chipId, 0, &pixRegs[Rd53aPixelCfg::toIndex(col, row)]);
            this->writeRegister(&Rd53a::PixPortal, pixRegs[Rd53aPixelCfg::toIndex(col, row)]);
            //if (pixRegs[Rd53aPixelCfg::toIndex(col, row)] != 0x0)
            //    std::cout << "[" << col << "][" << row << "] = 0x" << std::hex << pixRegs[Rd53aPixelCfg::toIndex(col, row)] << std::dec << std::endl;
            //if (row % 24 == 0)
            //    while(!core->isCmdEmpty()){;}
        }
        while(!core->isCmdEmpty()){;}
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
        if (counter == 100 ) {
            while(!core->isCmdEmpty()){;}
            counter = 0;
        }
    }
    while(!core->isCmdEmpty()){;}
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

int Rd53a::checkCom() {
     //std::cout << __PRETTY_FUNCTION__ << " : Checking communication for " << this->name << " by reading a register .." << std::endl;
    uint32_t regAddr = 21;
    uint32_t regValue = m_cfg[regAddr];
    rdRegister(m_chipId, regAddr);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    //std::cout << __PRETTY_FUNCTION__ << " : Trying to read data .." << std::endl;
    // TODO not happy about this, rx knowledge should not be here
    RawData *data = m_rxcore->readData();

    if (data != NULL) {
        if (!(data->words == 2 || data->words == 4)) {
            std::cout << "#ERROR# Received wrong number of words (" << data->words << ") for " << this->name << std::endl;
            return 0;
        }
        std::pair<uint32_t, uint32_t> answer = decodeSingleRegRead(data->buf[0], data->buf[1]);
        //std::cout << "Addr (" << answer.first << ") Value(" << answer.second << ")" << std::endl;
        
        if (answer.first != regAddr || answer.second != regValue) {
            std::cout << "#ERROR# Received data was not as expected:" << std::endl;
            std::cout << "    Received Addr: " << answer.first << " (expected " << regAddr <<")" << std::endl;
            std::cout << "    Received Value: " << answer.second << " (expected "<< regValue << ")" << std::endl;
            return 0;
        }

        //std::cout << "    ... success!" << std::endl;
        return 1;
    } else {
        std::cout << "#ERROR# Did not receive any data for " << this->name << std::endl;
        return 0;
    }
}

std::pair<uint32_t, uint32_t> decodeSingleRegRead(uint32_t higher, uint32_t lower) {
    if ((higher & 0x55000000) == 0x55000000) {
        return std::make_pair((lower>>16)&0x3FF, lower&0xFFFF);
    } else if ((higher & 0x99000000) == 0x99000000) {
        return std::make_pair((higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
    } else {
        std::cout << "#ERROR# Could not decode reg read!" << std::endl;
        return std::make_pair(999, 666);
    }
    return std::make_pair(999, 666);
}



