// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Library
// # Comment: Combines ITkPixV1 and CROCv1
// # Date: May 2020
// ################################

#include "AllChips.h"
#include "Rd53b.h"

#include "logging.h"

// Create logger
namespace {
  auto logger = logging::make_log("Rd53b");
}

bool rd53b_registred =
    StdDict::registerFrontEnd("RD53B", [](){return std::unique_ptr<FrontEnd>(new Rd53b());});

Rd53b::Rd53b() : FrontEnd(), Rd53bCfg(), Rd53bCmd(){
    txChannel = 99;
    rxChannel = 99;
    enforceChipIdInName = true;
    active = true;
    geo.nRow = 384;
    geo.nCol = 400;
}

Rd53b::Rd53b(HwController *core) : FrontEnd(), Rd53bCfg(), Rd53bCmd(core) {
    m_rxcore = core;
    txChannel = 99;
    rxChannel = 99;
    enforceChipIdInName = true;
    active = true;
    geo.nRow = 384;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

Rd53b::Rd53b(HwController *core, unsigned arg_channel) : FrontEnd(), Rd53bCfg(), Rd53bCmd(core){
    m_rxcore = core;
    txChannel = arg_channel;
    rxChannel = arg_channel;
    enforceChipIdInName = true;
    active = true;
    geo.nRow = 384;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

void Rd53b::init(HwController *core, unsigned arg_txChannel, unsigned arg_rxChannel) {
    this->setCore(core);
    m_rxcore = core;
    txChannel = arg_txChannel;
    rxChannel = arg_rxChannel;
    enforceChipIdInName = true;
    geo.nRow = 384;
    geo.nCol = 400;
    core->setClkPeriod(6.25e-9);
}

void Rd53b::resetAll() {
    logger->debug("Performing hard reset ...");
    // Send low number of transitions for at least 10us to put chip in reset state 
    logger->debug(" ... asserting CMD reset via low activity");
    for (unsigned int i=0; i<85; i++) {
        // Pattern corresponds to approx. 0.83MHz
        core->writeFifo(0xFFFFFFFF);
        core->writeFifo(0xFFFFFFFF);
        core->writeFifo(0xFFFFFFFF);
        core->writeFifo(0x00000000);
        core->writeFifo(0x00000000);
        core->writeFifo(0x00000000);
    }
    core->releaseFifo();
    while(!core->isCmdEmpty()){;}
    
    // Wait for at least 1000us before chip is release from reset
    logger->debug(" ... waiting for CMD reset to be released");
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Sync CMD decoder
    logger->debug(" ... send syncs");
    for(unsigned int i=0; i<32; i++)
        core->writeFifo(0x817E817E);
    core->releaseFifo();
    while(!core->isCmdEmpty()){;}

}

void Rd53b::configure() {
    this->configureInit();
    this->configureGlobal();
    this->configurePixels();
}

void Rd53b::enableAll() {
    logger->info("Resetting enable/hitbus pixel mask to all enabled!");
    for (unsigned int col = 0; col < n_Col; col++) {
        for (unsigned row = 0; row < n_Row; row ++) {
            setEn(col, row, 1);
            setHitbus(col, row, 1);
        }
    }
}

void Rd53b::configureInit() {
    logger->debug("Initiliasing chip ...");
    
    // Enable register writing to do more resetting
    logger->debug(" ... set global register in writeable mode");
    this->writeRegister(&Rd53b::GcrDefaultConfig, 0xAC75);
    this->writeRegister(&Rd53b::GcrDefaultConfigB, 0x538A);
    while(!core->isCmdEmpty()){;}

    // Send a global pulse to reset multiple things
    logger->debug(" ... send resets via global pulse");
    this->writeRegister(&Rd53b::GlobalPulseConf, 0x0FFF);
    this->writeRegister(&Rd53b::GlobalPulseWidth, 10);
    while(!core->isCmdEmpty()){;}
    this->sendGlobalPulse(m_chipId);
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    // Reset register
    this->writeRegister(&Rd53b::GlobalPulseConf, 0);

    
    // Reset Core
    logger->debug("Reset Cores!");
    uint16_t tmpRstCoreCol0 = this->RstCoreCol0.read();
    uint16_t tmpRstCoreCol1 = this->RstCoreCol1.read();
    uint16_t tmpRstCoreCol2 = this->RstCoreCol2.read();
    uint16_t tmpRstCoreCol3 = this->RstCoreCol3.read();

    uint16_t tmpEnCoreCol0 = this->EnCoreCol0.read();
    uint16_t tmpEnCoreCol1 = this->EnCoreCol1.read();
    uint16_t tmpEnCoreCol2 = this->EnCoreCol2.read();
    uint16_t tmpEnCoreCol3 = this->EnCoreCol3.read();
    
    // TODO this could be problematic for low power config
    for (unsigned i=0; i<16; i++) {
        this->writeRegister(&Rd53b::RstCoreCol0, 1<<i);
        this->writeRegister(&Rd53b::RstCoreCol1, 1<<i);
        this->writeRegister(&Rd53b::RstCoreCol2, 1<<i);
        this->writeRegister(&Rd53b::RstCoreCol3, 1<<i);
        this->writeRegister(&Rd53b::EnCoreCol0, 1<<i);
        this->writeRegister(&Rd53b::EnCoreCol1, 1<<i);
        this->writeRegister(&Rd53b::EnCoreCol2, 1<<i);
        this->writeRegister(&Rd53b::EnCoreCol3, 1<<i);
        while(!core->isCmdEmpty()){;}
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        this->sendClear(m_chipId);
        while(!core->isCmdEmpty()){;}
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
        
    this->writeRegister(&Rd53b::RstCoreCol0, tmpRstCoreCol0);
    this->writeRegister(&Rd53b::RstCoreCol1, tmpRstCoreCol1);
    this->writeRegister(&Rd53b::RstCoreCol2, tmpRstCoreCol2);
    this->writeRegister(&Rd53b::RstCoreCol3, tmpRstCoreCol3);
    this->writeRegister(&Rd53b::EnCoreCol0, tmpEnCoreCol0);
    this->writeRegister(&Rd53b::EnCoreCol1, tmpEnCoreCol1);
    this->writeRegister(&Rd53b::EnCoreCol2, tmpEnCoreCol2);
    this->writeRegister(&Rd53b::EnCoreCol3, tmpEnCoreCol3);
    
    // Send a clear cmd
    logger->debug(" ... sending clear command");
    this->sendClear(m_chipId);
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    logger->debug("Chip initialisation done!");
}

void Rd53b::configureGlobal() {
    // Read current threshold values
    uint16_t tmpTh1L = this->DiffTh1L.read();
    uint16_t tmpTh1R = this->DiffTh1R.read();
    uint16_t tmpTh1M = this->DiffTh1M.read();
    uint16_t tmpTh2 = this->DiffTh2.read();
    // Set high threshold during config
    this->writeRegister(&Rd53b::DiffTh1L, 500);
    this->writeRegister(&Rd53b::DiffTh1R, 500);
    this->writeRegister(&Rd53b::DiffTh1M, 500);
    this->writeRegister(&Rd53b::DiffTh2, 0);
    while(!core->isCmdEmpty()){;}

    logger->debug("Configuring all registers ...");
    for (unsigned addr=0; addr<numRegs; addr++) {
        this->sendWrReg(m_chipId, addr, m_cfg[addr]);
        
        // Special handling of preamp register
        if (addr == 13) { // specifically wait after setting preamp bias
            while(!core->isCmdEmpty()){;}
            std::this_thread::sleep_for(std::chrono::microseconds(5000));
        }

        if (addr % 20 == 0) // Wait every 20 regs to not overflow a buffer
            while(!core->isCmdEmpty()){;}
    }
    while(!core->isCmdEmpty()){;}
    
    this->writeRegister(&Rd53b::DiffTh1L, tmpTh1L);
    this->writeRegister(&Rd53b::DiffTh1R, tmpTh1R);
    this->writeRegister(&Rd53b::DiffTh1M, tmpTh1M);
    this->writeRegister(&Rd53b::DiffTh2, tmpTh2);
    
    while(!core->isCmdEmpty()){;}
}

void Rd53b::configurePixels() {
    logger->debug("Configure all pixel registers ...");
    // Setup pixel programming
    this->writeRegister(&Rd53b::PixAutoRow, 1);
    this->writeRegister(&Rd53b::PixBroadcast, 0);
    // Writing two columns and six rows at the same time
    for (unsigned dc=0; dc<n_DC; dc++) {
        this->writeRegister(&Rd53b::PixRegionCol, dc);
        this->writeRegister(&Rd53b::PixRegionRow, 0);
        for (unsigned row=0; row<n_Row; row++) {
            this->writeRegister(&Rd53b::PixPortal, pixRegs[dc][row]);
            if (row%32==0)
                while(!core->isCmdEmpty()){;}
        }
        while(!core->isCmdEmpty()){;}
    }
}

void Rd53b::configurePixels(std::vector<std::pair<unsigned, unsigned>> &pixels) {
    logger->debug("Configuring some pixel registers ...");
    // Writing two columns and six rows at the same time
    unsigned old_dc = 99999;
    unsigned write_counter = 0;
    this->writeRegister(&Rd53b::PixAutoRow, 0);
    this->writeRegister(&Rd53b::PixBroadcast, 0);
    for (auto &pixel: pixels) {
        if (old_dc != pixel.first/2) {
            this->writeRegister(&Rd53b::PixRegionCol, pixel.first>>1);
            old_dc = pixel.first/2;
        }
        this->writeRegister(&Rd53b::PixRegionRow, pixel.second); 
        this->writeRegister(&Rd53b::PixPortal, pixRegs[pixel.first>>1][pixel.second]);
        write_counter++;
        if (write_counter >= 20) {
            while(!core->isCmdEmpty()){;}
            write_counter = 0;
        }
    }
    while(!core->isCmdEmpty()){;}
}

void Rd53b::writeRegister(Rd53bReg Rd53bGlobalCfg::*ref, uint16_t value) {
    (this->*ref).write(value);
    logger->debug("Writing register {} with {}", (this->*ref).addr(), m_cfg[(this->*ref).addr()]);
    this->sendWrReg(m_chipId, (this->*ref).addr(), m_cfg[(this->*ref).addr()]);
}

void Rd53b::readRegister(Rd53bReg Rd53bGlobalCfg::*ref) {
    logger->debug("Reading register {}", (this->*ref).addr());
    this->sendRdReg(m_chipId, (this->*ref).addr());
}

void Rd53b::writeNamedRegister(std::string name, uint16_t value) {
    if(regMap.find(name) != regMap.end()) {
        logger->debug("Write named register {} -> {}", name, value);
        this->writeRegister(regMap[name], value);
    } else if(virtRegMap.find(name) != virtRegMap.end()) {
        logger->debug("Write named virtual register {} -> {}", name, value);
        this->writeRegister(virtRegMap[name], value);
    } else {
        logger->error("Trying to write named register, register not found: {}", name);
    }
}

Rd53bReg Rd53bGlobalCfg::*  Rd53b::getNamedRegister(std::string name) {
    if(regMap.find(name) != regMap.end()) {
        return regMap[name];
    } else if(virtRegMap.find(name) != virtRegMap.end()) {
        return virtRegMap[name];
    } else {
        logger->error("Trying to get named register, register not found: {}", name);
    }
    return NULL;
}

int Rd53b::checkCom() {
    if (this->ServiceBlockEn.read() == 0) {
        logger->error("Register messages not enabled, can't check communication ... proceeding blind! (Set \"ServiceBlockEn\" to 1 in the chip config)");
        return 1;
    }
    
    logger->debug("Checking communication for {} by reading a register ...", this->name);
    uint32_t regAddr = 21;
    uint32_t regValue = m_cfg[regAddr];
    this->sendRdReg(m_chipId, regAddr);
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
        unsigned size = data->getSize();       
        if (!(size == 2 || size == 4 || size == 8 || size == 12 || size == 6)) {
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

bool Rd53b::hasValidName() {

    // return true if no check is requested
    if(auto cfg = dynamic_cast<Rd53bCfg*>(this); !cfg->checkChipIdInName()) {
        return true;
    }

    // Rd53b stores serial numbers in on-chip registers, so service blocks be
    // enabled in order to query them
    if (this->ServiceBlockEn.read() == 0) {
        logger->error("Register messages not enabled, can't check chip id (set \"ServiceBlockEn\" to 1 in chip config");
        return false;
    }

    // if user is requested to enforce that the chip id be in the FrontEnd "name"
    // field, then readback the E-fuses to get the actual chip's ID
    itkpix_efuse_codec::EfuseData efuse_data = this->readEfuses();
    std::stringstream id_from_efuse;
    id_from_efuse << std::hex << efuse_data.chip_sn();
    bool id_in_name = name.find(id_from_efuse.str()) != std::string::npos;
    if(!id_in_name) {
        logger->error("Chip serial number from e-fuse data (0x{:x}) does not appear in Chip \"name\" field (\"{}\") in loaded configuration  for chip with ChipId = {}", efuse_data.chip_sn(), name, m_chipId);
        return false;
    }
    logger->info("Chip serial number obtained from e-fuse data: 0x{:x} (decoded e-fuse data: 0x{:x})", efuse_data.chip_sn(), efuse_data.raw());
    return true;
}

std::pair<uint32_t, uint32_t> Rd53b::decodeSingleRegRead(uint32_t higher, uint32_t lower) {
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

itkpix_efuse_codec::EfuseData Rd53b::readEfuses() {

    //
    // put E-fuse programmer circuit block into READ mode
    //
    this->writeRegister(&Rd53b::EfuseConfig, 0x0f0f);
    while(!core->isCmdEmpty()) {}

    //
    // send E-fuse circuit the reset signal to halt any other state (reset E-fuse block FSM)
    //
    //this->writeRegister(&Rd53b::GlobalPulseConf, 0x100);
    //this->writeRegister(&Rd53b::GlobalPulseWidth, 200);
    //while(!core->isCmdEmpty()) {}
    //this->sendGlobalPulse(m_chipId);

    //
    // read back the E-fuse registers
    //
    uint32_t efuse_data_0 = 0;
    uint32_t efuse_data_1 = 0;
    try {
        efuse_data_0 = readSingleRegister(&Rd53b::EfuseReadData0);
        efuse_data_1 = readSingleRegister(&Rd53b::EfuseReadData1);
    } catch (std::exception& e) {
        logger->warn("Failed to readback E-fuse data for chip with {}, exception received: {}", m_chipId, e.what());
        return itkpix_efuse_codec::EfuseData{0};
    }
    uint32_t efuse_data = ((efuse_data_1 & 0xffff) << 16) | (efuse_data_0 & 0xffff);

    // decode the e-fuse data (performs single-bit error-correction)
    std::string decoded_efuse_binary_str = itkpix_efuse_codec::decode(efuse_data);
    return itkpix_efuse_codec::EfuseData{decoded_efuse_binary_str};
}

uint32_t Rd53b::readSingleRegister(Rd53bReg Rd53bGlobalCfg::*ref) {
    // send a read register command to the chip so that it
    // sends back the current value of the register
    this->sendRdReg(m_chipId, (this->*ref).addr());
    while(!core->isCmdEmpty()) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(1)) ;

    // go through the incoming data stream and get the register read data
    std::vector<RawDataPtr> dataVec = m_rxcore->readData();
    RawDataPtr data;
    if (dataVec.size() > 0) {
        data = dataVec[0];
    }
    
    if(data != NULL) {
        if(!(data->getSize() >= 2)) {
            logger->warn("readSingleRegister failed, received wrong number of words ({}) for FE with chipId {}", data->getSize(), m_chipId);
            return 0;
        }

        auto [received_address, register_value] = Rd53b::decodeSingleRegRead(data->get(0), data->get(1));
        if(received_address != (this->*ref).addr()) {
            logger->warn("readSingleRegister failed, returned data is for unexpected register address (received address: {}, expected address {})", received_address, (this->*ref).addr());
            return 0;
        }
        return register_value;
    }

    logger->warn("readSingleRegister failed, did not received register readback data from chip with chipId {}", m_chipId);
    return 0;
}
    
void Rd53b::confAdc(uint16_t MONMUX, bool doCur) {
    //This only works for voltage MUX values.
    uint16_t OriginalGlobalRT = this->GlobalPulseConf.read();
    uint16_t OriginalMonitorEnable = this->MonitorEnable.read(); //Enabling monitoring
    uint16_t OriginalMonitorV = this->MonitorV.read();
    uint16_t OriginalMonitorI = this->MonitorI.read();

    if (doCur)
    {
        this->writeRegister(&Rd53b::MonitorV, 1);      // Forward via VMUX
        this->writeRegister(&Rd53b::MonitorI, MONMUX); // Select what to monitor
    }
    else
    {
        this->writeRegister(&Rd53b::MonitorV, MONMUX); // Select what to monitor
    }

    this->writeRegister(&Rd53b::MonitorEnable, 1); // Enabling monitoring
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    this->writeRegister(&Rd53b::GlobalPulseConf, 0x40); // Reset ADC
    this->writeRegister(&Rd53b::GlobalPulseWidth, 4);   // Duration = 4 inherited from RD53A
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    this->sendGlobalPulse(m_chipId);
    std::this_thread::sleep_for(std::chrono::microseconds(1000000)); // Need to wait long enough for ADC to reset

    this->writeRegister(&Rd53b::GlobalPulseConf, 0x1000); //Trigger ADC Conversion
    while (!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    this->sendGlobalPulse(m_chipId);
    std::this_thread::sleep_for(std::chrono::microseconds(1000)); //This is neccessary to clean. This might be controller dependent.

    // Reset register values
    this->writeRegister(&Rd53b::GlobalPulseConf, OriginalGlobalRT);
    this->writeRegister(&Rd53b::MonitorEnable, OriginalMonitorEnable);
    this->writeRegister(&Rd53b::MonitorV, OriginalMonitorV);
    this->writeRegister(&Rd53b::MonitorI, OriginalMonitorI);
    while (!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));
}

void Rd53b::runRingOsc(uint16_t duration, bool isBankB) {
    uint16_t OriginalGlobalRT = this->GlobalPulseConf.read();

    this->writeRegister(&Rd53b::GlobalPulseConf, isBankB ? 0x4000 : 0x2000); //Ring Osc Enable Rout
    this->writeRegister(&Rd53b::GlobalPulseWidth, duration);
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    this->sendGlobalPulse(m_chipId);

    std::this_thread::sleep_for(std::chrono::milliseconds(1)); //This is neccessary to clean. This might be controller dependent.

    this->writeRegister(&Rd53b::GlobalPulseConf, OriginalGlobalRT); // Recover the original routing
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));
}
