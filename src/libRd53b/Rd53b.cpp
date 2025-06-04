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
#include "HwController.h"

#include "logging.h"

// Create logger
namespace {
  auto logger = logging::make_log("Rd53b");
}

bool rd53b_registred =
    StdDict::registerFrontEnd("RD53B", [](){return std::unique_ptr<FrontEnd>(new Rd53b());});

Rd53b::Rd53b() : FrontEnd(), Rd53bCfg(), Rd53bCmd(){
    enforceChipIdInName = true;
    active = true;
    geo.nRow = 384;
    geo.nCol = 400;
}

void Rd53b::init(HwController *core, const FrontEndConnectivity& fe_cfg){
    this->setCore(core);
    m_rxcore = core;
    initFeConnectivity(fe_cfg);
    core->setClkPeriod(6.25e-9);
}

void Rd53b::resetAllHard() {
    logger->debug("Performing hard reset ...");
    // Send low number of transitions for at least 10us to put chip in reset state
    logger->debug(" ... asserting CMD reset via low activity");
    for (unsigned int i=0; i<85; i++) {
        // Pattern corresponds to approx. 0.83MHz (192 bits @ 160 Mb/s)
        // 85 times means sending this signal for approx. 102us, that is >> 10us
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

void Rd53b::resetAllSoft() {
    logger->info("Performing soft reset ...");

    this->writeRegister(&Rd53b::GlobalPulseConf, 0x490);
    this->writeRegister(&Rd53b::GlobalPulseWidth, 10);
    while(!core->isCmdEmpty()){;}

    this->sendGlobalPulse(m_chipId);
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

}

void Rd53b::configure() {
    this->configureInit();
    this->configureGlobal();
    this->configurePixels();
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

void Rd53b::configurePixelMaskParallel() {
    logger->debug("Configure all pixel mask regs in parallel ...");
    // Setup pixel programming
    this->writeRegister(&Rd53b::PixAutoRow, 1);
    this->writeRegister(&Rd53b::PixConfMode, 0);
    this->writeRegister(&Rd53b::PixBroadcast, 1);
    // Writing all core columns at the same time, loop over dc in core
    for (unsigned dc=0; dc<4; dc++) {
        this->writeRegister(&Rd53b::PixRegionCol, dc);
        this->writeRegister(&Rd53b::PixRegionRow, 0);
        std::array<uint16_t, n_Row> maskBits;
        for (unsigned row=0; row<n_Row; row++) {
            maskBits[row] = toTenBitMask(pixRegs[dc][row]);
        }
        this->sendPixRegBlock(m_chipId, maskBits);
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

yarrStatus Rd53b::writeRegister(Rd53bRegDefault Rd53bGlobalCfg::*ref, uint16_t value) {
    (this->*ref).write(value);
    logger->debug("Writing register {} with {}", (this->*ref).addr(), m_cfg[(this->*ref).addr()]);
    this->sendWrReg(m_chipId, (this->*ref).addr(), m_cfg[(this->*ref).addr()]);
    return yarrSuccess;
}

yarrStatus Rd53b::readRegister(Rd53bRegDefault Rd53bGlobalCfg::*ref, uint16_t &value, uint8_t &chipId) {
    m_rxcore->flushBuffer();
    // send a read register command to the chip so that it
    // sends back the current value of the register
    this->sendRdReg(m_chipId, (this->*ref).addr());
    while(!core->isCmdEmpty()) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // go through the incoming data stream and get the register read data
    std::vector<RawDataPtr> dataVec;
    RawDataPtr data;
    bool found = false;
    double check_seconds = 0.;

    logger->debug("Reading register data for chip ID {} on channel {}", m_chipId, regRxChannel);
    std::chrono::steady_clock::time_point comm_t0 = std::chrono::steady_clock::now();

    do{
      dataVec = m_rxcore->readData();
      if (dataVec.size() > 0) {
        for(auto const &v : dataVec) {
            // Find raw data for this address
	  if (regRxChannel != v->getAdr()){
	    logger->debug("Data doesn't belong to the regRx channel {}, instead comes from channel {}", regRxChannel, v->getAdr());
	    continue;
	  }

	  if (v->get(0) != 0xffffdead) {
	    data = v;
	    if(!(data->getSize() >= 2)) {
	      logger->error("readRegister failed, received wrong number of words ({}) for FE with chipId {}", data->getSize(), m_chipId);
	      continue;
	    }

	    auto [id, received_address, register_value] = Rd53b::decodeSingleRegReadID(data->get(0), data->get(1));
	    chipId = id; // chipId is read from the chip wirebonded ID, m_chipId is set in the chip config file
	    if(m_chipId > 15 || id == (m_chipId&0x3)) { // only compare if not broadcasting
	      if(received_address != (this->*ref).addr()) {
		logger->error("readRegister failed, returned data is for unexpected register address (received address: {}, expected address {})", received_address, (this->*ref).addr());
		continue;
	      }
	      logger->debug("readRegister successful for register address {} with value {} from chip with chipId {}", (this->*ref).addr(), register_value, m_chipId);
	      found = true;
	      // Update memory
	      m_cfg[(this->*ref).addr()] = register_value;
	      // Return value
	      value = (this->*ref).read();
	      return yarrSuccess;
	    } else {
	      logger->info("readRegister 0x{:x} 0x{:x} -> ID {} - {}, addr 0x{:x} val 0x{:x}", data->get(0), data->get(1), id, m_chipId&0x3, received_address, register_value);
	      logger->info("Sending another readRegister command.");
	      m_rxcore->flushBuffer();
	      this->sendRdReg(m_chipId, (this->*ref).addr());
	      while(!core->isCmdEmpty()) {}
	      break;
	    }
	  }
        }
      }
      else{
	logger->debug("No raw data received.");
      }

      std::chrono::steady_clock::time_point comm_t1 = std::chrono::steady_clock::now();
      check_seconds = std::chrono::duration_cast<std::chrono::seconds>(comm_t1 - comm_t0).count();
      if(!found)
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }while(!found && check_seconds<3.);

    logger->warn("readRegister failed, did not received register readback data for address {} from chip with chipId {}", (this->*ref).addr(), m_chipId);

    value = 65535;
    return yarrFailure;
}

yarrStatus Rd53b::readRegister(Rd53bRegDefault Rd53bGlobalCfg::*ref, uint16_t &value) {
  uint8_t _ = 0;
  return readRegister(ref, value, _);
}

yarrStatus Rd53b::readUpdateWriteRegister(Rd53bRegDefault Rd53bGlobalCfg::*ref, const uint16_t value) {
   	uint16_t tmp;    
    // Update reg in memory
    if  (readRegister(ref, tmp) != yarrSuccess) {
        logger->error("Failed to read update register!");
        return yarrFailure;
    }
    // Write register 
    if (writeRegister(ref, value) != yarrSuccess) {
        logger->error("Failed to write register after updating!");
        return yarrFailure;
    }

    return yarrSuccess;
}
    

yarrStatus Rd53b::writeNamedRegister(std::string name, const uint16_t value) {
    if(regMap.find(name) != regMap.end()) {
        logger->debug("Write named register {} -> {}", name, value);
        this->writeRegister(regMap[name], value);
	while(!core->isCmdEmpty()){;}
	std::this_thread::sleep_for(std::chrono::microseconds(100));
        return yarrSuccess;
    }
    
    if(virtRegMap.find(name) != virtRegMap.end()) {
        logger->debug("Write named virtual register {} -> {}", name, value);
        this->writeRegister(virtRegMap[name], value);
	while(!core->isCmdEmpty()){;}
	std::this_thread::sleep_for(std::chrono::microseconds(100));
        return yarrSuccess;
    } 

    logger->error("Trying to write named register, register not found: {}", name);
    return yarrFailure;
}

yarrStatus Rd53b::readNamedRegister(std::string name, uint16_t &value) {
    if(regMap.find(name) != regMap.end()) {
        logger->debug("Read named register {}", name);
        if (this->readRegister(regMap[name], value) != yarrSuccess) {
            logger->error("Failed to read register {}", name);
            return yarrFailure; 
        }
        return yarrSuccess;
    }
    
    logger->error("Trying to read named register, register not found: {}", name);
    return yarrFailure;
}

yarrStatus Rd53b::readUpdateWriteNamedRegister(std::string name, const uint16_t value) {
    if(regMap.find(name) != regMap.end()) {
        logger->debug("Local update named register {} with {}", name, value);
        if (this->readUpdateWriteRegister(regMap[name], value) != yarrSuccess) {
            logger->error("Failed to read update write named register {}", name);
            return yarrFailure;
        }
        return yarrSuccess;
    } 
    
    logger->error("Trying to local update named register, register not found: {}", name);
    return yarrFailure;
}


Rd53bRegDefault Rd53bGlobalCfg::*  Rd53b::getNamedRegisterObject(std::string name) {
    if(regMap.find(name) != regMap.end()) {
        return regMap[name];
    } else if(virtRegMap.find(name) != virtRegMap.end()) {
        return virtRegMap[name];
    } else {
        logger->error("Trying to get named register, register not found: {}", name);
    }
    return NULL;
}

yarrStatus Rd53b::setNamedRegister(std::string name, const uint16_t value){
    logger->debug("Set virtual register {} -> {}", name, value);
    if(regMap.find(name) != regMap.end()) {
        (this->*regMap[name]).write(value);
        return yarrSuccess;
    }
    logger->error("Trying to set named register, register not found: {}", name);
    return yarrFailure;
}

yarrStatus Rd53b::getNamedRegister(std::string name, uint16_t &value){
    logger->debug("Get virtual register value {}", name);
    if(regMap.find(name) != regMap.end()) {
        value = (this->*regMap[name]).read();
        return yarrSuccess;
    }
    logger->error("Trying to get named register, register not found: {}", name);
    return yarrFailure;
}

yarrStatus Rd53b::checkCom() {
    if (this->ServiceBlockEn.read() == 0) {
        logger->error("Register messages not enabled, can't check communication ... proceeding blind! (Set \"ServiceBlockEn\" to 1 in the chip config)");
        return yarrSuccess;
    }
    
    logger->debug("Checking communication for {} by reading a register ...", this->name);
    uint32_t regAddr = 21;
    uint32_t regValue = m_cfg[regAddr];
  
    m_rxcore->flushBuffer();
    this->sendRdReg(m_chipId, regAddr);
    while(!core->isCmdEmpty()){;} // Required by the rdRegister() above 
                                  // (when relying on isCmdEmpty() to actually send commands).
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // TODO not happy about this, rx knowledge should not be here
    std::vector<RawDataPtr> dataVec = m_rxcore->readData();
    RawDataPtr data;

    if (dataVec.size() > 0 && dataVec[0]->getAdr() == regRxChannel) {
        data = dataVec[0];
    }

    if (data != NULL) {
        unsigned size = data->getSize();       
        if (!(size == 2 || size == 4 || size == 8 || size == 12 || size == 6)) {
            logger->error("Received wrong number of words ({}) for {}", data->getSize(), this->name);
            return yarrFailure;
        }
        std::pair<uint32_t, uint32_t> answer = decodeSingleRegRead(data->get(0), data->get(1));
        logger->debug("Addr ({}) Value({})", answer.first, answer.second);

        if (answer.first != regAddr || answer.second != regValue) {
            logger->error("Received data was not as expected:");
            logger->error("    Received Addr: {} (expected {})", answer.first, regAddr);
            logger->error("    Received Value: {} (expected {})", answer.second, regValue);
            return yarrFailure;
        }

        logger->debug("... success");
        return yarrSuccess;
    } else {
        logger->error("Did not receive any data for {}", this->name);
        return yarrFailure;
    }
}

yarrStatus Rd53b::hasValidName() {
    // return true if no check is requested
    if(auto cfg = dynamic_cast<Rd53bCfg*>(this); !cfg->checkChipIdInName()) {
        return yarrSuccess;
    }

    uint32_t efuse = this->getEfuses();
    std::stringstream id_from_efuse;
    id_from_efuse << std::hex << efuse;

    bool id_in_name = name.find(id_from_efuse.str()) != std::string::npos;

    if(!id_in_name) {
        logger->error("Chip serial number decoded from e-fuse data (0x{:x}) does not appear in Chip \"name\" field (\"{}\") in loaded configuration  for chip with ChipId = {}", efuse, name, m_chipId);
       	return yarrFailure;
    }
    logger->info("Matched chip serial number: 0x{:x}", efuse );
    return yarrSuccess;
}


uint32_t Rd53b::getEfuses() {
    // Rd53b stores serial numbers in on-chip registers, so service blocks be
    // enabled in order to query them
    if (this->ServiceBlockEn.read() == 0) {
        logger->error("Register messages not enabled, can't check chip id (set \"ServiceBlockEn\" to 1 in chip config");
        return yarrFailure;
    }

    // if user is requested to enforce that the chip id be in the FrontEnd "name"
    // field, then readback the E-fuses to get the actual chip's ID
    uint32_t efuse_data_raw = this->readEfusesRaw();
    logger->info("Chip serial number obtained from e-fuse data (raw): 0x{:x}", efuse_data_raw);

    itkpix_efuse_codec::EfuseData efuse_data = itkpix_efuse_codec::EfuseData{itkpix_efuse_codec::decode(efuse_data_raw)};
    itkpix_efuse_codec::EfuseData efuse_data_old = itkpix_efuse_codec::EfuseData{itkpix_efuse_codec::decodeOldFormat(efuse_data_raw)};

    uint32_t chip_sn = efuse_data.chip_sn();
    uint32_t chip_sn_old = efuse_data_old.chip_sn();

    // Test for error correction
    // ITkPixV1 wafer probing used two different parity encoding and switched in the beginning of pre-production
    if (chip_sn != ((efuse_data_raw >> 8) & 0xFFFFF)) { 
        logger->warn("Chip serial number decoded from e-fuse did not match parity bits, tried to error correct.");
        if (chip_sn_old != ((efuse_data_raw >> 8) & 0xFFFFF)) {
            logger->warn("Also decoding with old format did not match parity bits, returning error corrected new version.");
            logger->info("Chip serial number obtained from e-fuse data: 0x{:x}", chip_sn );
            return chip_sn;
        } else {
            // This is likely the case for wafers probed before Oct 2021 (wafer ID is not chronological)
            logger->info("Decoding efuse with old decoding did not have parity errors, using old decoding!");
            logger->info("Chip serial number decoded with old format from e-fuse data: 0x{:x}", chip_sn_old);
            return chip_sn_old;
        }
    } else {
        logger->info("Chip serial number obtained from e-fuse data: 0x{:x}", chip_sn );
        return chip_sn;
    }
    
    return yarrFailure;
}

std::pair<uint32_t, uint32_t> Rd53b::decodeSingleRegRead(uint32_t higher, uint32_t lower) {
    std::pair<uint32_t, uint32_t> output = std::make_pair(999, 666);
    if ((higher & 0x55000000) == 0x55000000) {
        output = std::make_pair((lower>>16)&0x3FF, lower&0xFFFF);
    } else if ((higher & 0x99000000) == 0x99000000) {
        output = std::make_pair((higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
    } else {
        logger->error("Could not decode reg read!");
        output = std::make_pair(999, 666);
    }
    return output;
}

std::tuple<uint8_t, uint32_t, uint32_t> Rd53b::decodeSingleRegReadID(uint32_t higher, uint32_t lower) {
    std::tuple<uint8_t, uint32_t, uint32_t> output = std::make_tuple(16, 999, 666);
    // only the 2 LSB of the chip ID are sent by the chip. Ref manual 10.2 Aurora and RD53B Data
    if ((higher & 0xFF000000) == 0x55000000) { // register address 136 which is EfuseReadData0
        output = std::make_tuple((higher>>22)&0x3, (lower>>16)&0x3FF, lower&0xFFFF);
    } else if ((higher & 0xFF000000) == 0x99000000) { // register address 135 which is EfuseReadData1
        output = std::make_tuple((higher>>22)&0x3, (higher>>10)&0x3FF, ((lower>>26)&0x3F)+((higher&0x3FF)<<6));
    } else {
        logger->error("Could not decode reg read!");
        output = std::make_tuple(16, 999, 666);
    }
    return output;
}

itkpix_efuse_codec::EfuseData Rd53b::readEfuses() {

    //
    // put E-fuse programmer circuit block into READ mode
    //
    this->writeRegister(&Rd53b::EfuseConfig, 0x0f0f);
    while(!core->isCmdEmpty()) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(100));     
    
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
    uint16_t efuse_data_0 = 0;
    uint16_t efuse_data_1 = 0;
    
    if (readRegister(&Rd53b::EfuseReadData0, efuse_data_0) != yarrSuccess) {
        logger->warn("Failed to readback E-fuse 0 data for chip with {}", m_chipId);
        return itkpix_efuse_codec::EfuseData{0};
    }

    if (readRegister(&Rd53b::EfuseReadData1, efuse_data_1) != yarrSuccess) {
        logger->warn("Failed to readback E-fuse 1 data for chip with {}", m_chipId);
        return itkpix_efuse_codec::EfuseData{0};
    }
    
    uint32_t efuse_data = (((uint32_t)efuse_data_1 & 0xffff) << 16) | ((uint32_t)efuse_data_0 & 0xffff);

    // decode the e-fuse data (performs single-bit error-correction)
    std::string decoded_efuse_binary_str = itkpix_efuse_codec::decode(efuse_data);
    return itkpix_efuse_codec::EfuseData{decoded_efuse_binary_str};
}

uint32_t Rd53b::readEfusesRaw() {

    //
    // put E-fuse programmer circuit block into READ mode
    //
    this->writeRegister(&Rd53b::EfuseConfig, 0x0f0f);
    while(!core->isCmdEmpty()) {}
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

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
    uint16_t efuse_data_0 = 0;
    uint16_t efuse_data_1 = 0;
    
    if (readRegister(&Rd53b::EfuseReadData0, efuse_data_0) != yarrSuccess) {
        logger->warn("Failed to readback E-fuse 0 data for chip with {}", m_chipId);
        return 0;
    }

    if (readRegister(&Rd53b::EfuseReadData1, efuse_data_1) != yarrSuccess) {
        logger->warn("Failed to readback E-fuse 1 data for chip with {}", m_chipId);
        return 0;
    }
    return (((uint32_t)efuse_data_1 & 0xffff) << 16) | ((uint32_t)efuse_data_0 & 0xffff);
}

uint8_t Rd53b::readChipId() {
    uint16_t _ = 0;
    uint8_t id = 15;
    if (readRegister(&Rd53b::EfuseReadData0, _, id) != yarrSuccess) {
        logger->warn("Failed to readback E-fuse 1 data for chip with {}", m_chipId);
        return 255;
    }
    return id;
}

yarrStatus Rd53b::confAdc(uint16_t MONMUX, bool doCur) {
    //This only works for voltage MUX values.
    uint16_t OriginalGlobalRT = this->GlobalPulseConf.read();
    uint16_t OriginalMonitorEnable = this->MonitorEnable.read(); //Enabling monitoring
    uint16_t OriginalMonitorV = this->MonitorV.read();
    uint16_t OriginalMonitorI = this->MonitorI.read();
    
    // this value is AND'ed with all write register functions
    // if ALL writes are successful, success=true and yarrSuccess is returned
    bool success = true;

    if (doCur)
    {
        success = success && (this->writeRegister(&Rd53b::MonitorV, 1) == yarrSuccess);      // Forward via VMUX
        success = success && (this->writeRegister(&Rd53b::MonitorI, MONMUX) == yarrSuccess); // Select what to monitor
    }
    else
    {
        success = success && (this->writeRegister(&Rd53b::MonitorV, MONMUX) == yarrSuccess); // Select what to monitor
    }

    success = success && (this->writeRegister(&Rd53b::MonitorEnable, 1) == yarrSuccess); // Enabling monitoring
    while(!core->isCmdEmpty()){;}

    success = success && (this->writeRegister(&Rd53b::GlobalPulseConf, 0x40) == yarrSuccess); // Reset ADC
    success = success && (this->writeRegister(&Rd53b::GlobalPulseWidth, 4) == yarrSuccess);   // Duration = 4 inherited from RD53A
    while(!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    this->sendGlobalPulse(m_chipId);
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Need to wait long enough for ADC to reset

    success = success && (this->writeRegister(&Rd53b::GlobalPulseConf, 0x1000) == yarrSuccess); //Trigger ADC Conversion
    while (!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    this->sendGlobalPulse(m_chipId);
    std::this_thread::sleep_for(std::chrono::microseconds(1000)); //This is neccessary to clean. This might be controller dependent.

    // Reset register values
    success = success && (this->writeRegister(&Rd53b::GlobalPulseConf, OriginalGlobalRT) == yarrSuccess);
    success = success && (this->writeRegister(&Rd53b::MonitorEnable, OriginalMonitorEnable) == yarrSuccess);
    success = success && (this->writeRegister(&Rd53b::MonitorV, OriginalMonitorV) == yarrSuccess);
    success = success && (this->writeRegister(&Rd53b::MonitorI, OriginalMonitorI) == yarrSuccess);
    while (!core->isCmdEmpty()){;}
    std::this_thread::sleep_for(std::chrono::microseconds(100));

    return (success ? yarrSuccess : yarrFailure);
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
