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

#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53a");
}

bool rd53a_registred =
    StdDict::registerFrontEnd("RD53A", [](){return std::unique_ptr<FrontEnd>(new Rd53a());});

Rd53a::Rd53a() : FrontEnd(), Rd53aCfg(), Rd53aCmd() {
    active = true;
    geo.nRow = 192;
    geo.nCol = 400;
}

void Rd53a::init(HwController *arg_core, const FrontEndConnectivity& fe_cfg) {
    this->setCore(arg_core);
    m_rxcore = arg_core;
    initFeConnectivity(fe_cfg);
    core->setClkPeriod(6.25e-9);
}

void Rd53a::enableAll() {
    logger->info("Resetting enable/hitbus pixel mask to all enabled!");
    for (unsigned int col = 0; col < n_Col; col++) {
        for (unsigned row = 0; row < n_Row; row ++) {
            setEn(col, row, 1);
            setHitbus(col, row, 1);
        }
    }
}

void Rd53a::resetAllHard() {
    this->configureInit();
}

void Rd53a::configure() {
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
    while(!core->isCmdEmpty()){;}
    this->globalPulse(m_chipId, 8);
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //Sync frames required by BDAQ
    for (int i=0;i<32;++i)
        this->sync();
    while(!core->isCmdEmpty()){;}
    this->writeRegister(&Rd53a::GlobalPulseRt, 0x4100); //activate monitor and prime sync FE AZ
    while(!core->isCmdEmpty()){;}
    this->globalPulse(m_chipId, 8);
    while(!core->isCmdEmpty()){;}
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
    logger->info("Write named register: {} -> {}", name, value);
    if (regMap.find(name) != regMap.end())
        writeRegister(regMap[name], value);
}

void Rd53a::setRegisterValue(std::string name, uint16_t value){
    logger->debug("Set virtual register {} -> {}", name, value);
    (this->*regMap[name]).write(value);
}

uint16_t Rd53a::getRegisterValue(std::string name){
    logger->debug("Get virtual register value {}", name);
    return (this->*regMap[name]).read();
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
        logger->error("Col {} out of range!", col);
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
        logger->error(" --> col ({}) out of range!", col);
    }
}

int Rd53a::checkCom() {
    logger->debug("Checking communication for {} by reading a register ...", this->name);
    uint32_t regAddr = 21;
    uint32_t regValue = m_cfg[regAddr];
    rdRegister(m_chipId, regAddr);
    while(!core->isCmdEmpty()){;} // Required by the rdRegister() above 
                                  // (when relying on isCmdEmpty() to actually send commands).
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // TODO not happy about this, rx knowledge should not be here
    std::vector<RawDataPtr> dataVec = m_rxcore->readData();
    RawDataPtr data;
    if (dataVec.size() > 0) {
        data = dataVec[0];
    }

    if (data != NULL) {
        if (!(data->getSize() == 2 || data->getSize() == 4 || data->getSize() == 8 || data->getSize() == 12 || data->getSize() == 6)) {
            logger->error("Received wrong number of words ({}) for {}", data->getSize(), this->name);
            return 0;
        }
        std::pair<uint32_t, uint32_t> answer = decodeSingleRegRead(data->get(0), data->get(1));
        logger->debug("Addr ({}) Value({})", answer.first, answer.second);

        if (answer.first != regAddr || answer.second != regValue) {
            logger->error("Received data was not as expected:");
            logger->error("    Received Addr: {} (expected {})", answer.first, regAddr);
            logger->error("    Received Value: {} (expected {})", answer.second, regValue);
            return 0;
        }

        logger->debug("... success");
        return 1;
    } else {
        logger->error("Did not receive any data for {}", this->name);
        return 0;
    }
}

std::pair<uint32_t, uint32_t> Rd53a::decodeSingleRegRead(uint32_t higher, uint32_t lower) {
    if ((higher & 0x55000000) == 0x55000000) {
        return std::make_pair((lower>>16)&0x3FF, lower&0xFFFF);
    } else if ((higher & 0x99000000) == 0x99000000) {
        return std::make_pair((higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
    } else {
        logger->error("Could not decode reg read!");
        return std::make_pair(999, 666);
    }
    return std::make_pair(999, 666);
}

void Rd53a::confADC(uint16_t MONMUX,bool doCur=false) {
    //This only works for voltage MUX values. 
    uint16_t OriginalGlobalRT = this->GlobalPulseRt.read();

    if(doCur) {
        this->writeRegister(&Rd53a::MonitorVmonMux,  11); //Forward via VMUX    
        this->writeRegister(&Rd53a::MonitorImonMux,  MONMUX); //Select what to monitor
    } else {
        this->writeRegister(&Rd53a::MonitorVmonMux,  MONMUX); //Select what to monitor    
    }

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

void Rd53a::runRingOsc(uint16_t duration) {
    uint16_t OriginalGlobalRT = this->GlobalPulseRt.read();

    this->writeRegister(&Rd53a::GlobalPulseRt ,0x2000); //Ring Osc Enable Rout
    this->idle();
    this->globalPulse(m_chipId, duration);  

    std::this_thread::sleep_for(std::chrono::milliseconds(1)); //This is neccessary to clean. This might be controller dependent.  

    this->writeRegister(&Rd53a::GlobalPulseRt ,OriginalGlobalRT); //Trigger ADC Conversion
}

