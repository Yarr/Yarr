// #################################
// # Project: Yarr
// # Description: StarChips Library
// # Comment: StarChip FrontEnd class
// ################################

#include "StarChips.h"
#include "StarChipsBroadcast.h"

#include <chrono>

#include "logging.h"

namespace {
  auto logger = logging::make_log("StarChips");
}

#include "AllChips.h"

bool star_chips_default_registered =
StdDict::registerFrontEnd
  ("Star", []() { return std::unique_ptr<FrontEnd>(new StarChips(0, 0)); });
bool star_chips_v0_registered =
StdDict::registerFrontEnd
  ("Star_vH0A0", []() { return std::unique_ptr<FrontEnd>(new StarChips(0, 0)); });
// a.k.a PPA
bool star_chips_v1_registered =
StdDict::registerFrontEnd
  ("Star_vH0A1", []() { return std::unique_ptr<FrontEnd>(new StarChips(1, 0)); });
// a.k.a PPB
bool star_chips_v1_both_registered =
StdDict::registerFrontEnd
  ("Star_vH1A1", []() { return std::unique_ptr<FrontEnd>(new StarChips(1, 1)); });

StarChips::StarChips(int abc_version, int hcc_version)
  : StarCfg(abc_version, hcc_version), StarCmd(), FrontEnd()
{
	m_txcore  = nullptr;

	txChannel = 99;
	rxChannel = 99;
	active = false;
	geo.nRow = 2;
	geo.nCol = 128;


	//Create dummy configuration as placeholder for globalFe in preScan routines
	setHCCChipId(0xf);
	addABCchipID(0xf, 0);
}

#if 0
StarChips::StarChips(HwController *arg_core)
: StarCmd(), FrontEnd()
{
	m_txcore  = arg_core;
	m_rxcore = arg_core;
	txChannel = 99;
	rxChannel = 99;
	active = true;
	geo.nRow = 2;
	geo.nCol = 128;

}

StarChips::StarChips(HwController *arg_core, unsigned arg_channel)
: StarCmd(), FrontEnd()
{
	m_txcore  = arg_core;
	m_rxcore = arg_core;
	txChannel = arg_channel;
	rxChannel = arg_channel;

	active = true;
	geo.nRow = 2;
	geo.nCol = 128;
}

StarChips::StarChips(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel)
: StarCmd(), FrontEnd()
{
	m_txcore  = arg_core;
	m_rxcore = arg_core;
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;

	active = true;
	geo.nRow = 2;
	geo.nCol = 128;
}
#endif

void StarChips::init(HwController *arg_core, unsigned arg_txChannel, unsigned arg_rxChannel) {
	logger->debug("Running init {} {} {}", (void*)arg_core, arg_txChannel, arg_rxChannel);
	m_txcore  = arg_core;
	m_rxcore = arg_core;
	txChannel = arg_txChannel;
	rxChannel = arg_rxChannel;
	active = true;

	active = true;
	geo.nRow = 2;
	geo.nCol = 128;
}

std::unique_ptr<FrontEnd>  StarChips::getGlobal() {
  return std::make_unique<StarChipsBroadcast>(m_abc_version, m_hcc_version);
}

void StarChips::setHccId(unsigned hccID) {
  //First step will consist in setting the HCC ID (serial number might be different depending on fuse !)
  //Load the eFuse serial number (and stop HPR)
  if(!m_txcore) {
    logger->warn("Set HCC ID (to {}) called before init", hccID);
    return;
  }

  // Before writing the ID, make sure the fuse ID is loaded
  sendCmd(write_hcc_register(16, 0x4, 0xf));

  //Let's reset the HCC communications ID.
  //  Use a broadcast write of the required ID+fuse on reg 17
  uint32_t newReg17val = (hccID<<28) | m_fuse_id;
  sendCmd(write_hcc_register(17, newReg17val, 0xf));
  logger->info("Set HCC ID to {} (sent on reg17 0x{:08x})", hccID, newReg17val);
}

void StarChips::resetHCCStars() {
    logger->debug("Sending fast command #{} HCC_REG_RESET", LCB::HCC_REG_RESET);
    this->sendCmd(LCB::fast_command(LCB::HCC_REG_RESET, 0) );
}

void StarChips::resetABCStars() {
	uint8_t delay = 0; //2 bits BC delay

	logger->debug("Sending fast command #{} ABC_REG_RESET", LCB::ABC_REG_RESET);
	sendCmd(LCB::fast_command(LCB::ABC_REG_RESET, delay) );

	logger->debug("Sending fast command #{} ABC_SLOW_COMMAND_RESET", LCB::ABC_SLOW_COMMAND_RESET);
	sendCmd(LCB::fast_command(LCB::ABC_SLOW_COMMAND_RESET, delay) );

	// TODO: This should be done somewhere, but only after we're
	//       also reading out the SEU status registers
	// logger->debug("Sending fast command #{} ABC_SEU_RESET", LCB::ABC_SEU_RESET);
	// sendCmd(LCB::fast_command(LCB::ABC_SEU_RESET, delay) );
}

void StarChips::resetAllHard(){
	logger->info("Global reseting all HCC and ABC on the same LCB control segment");

    logger->debug("Sending fast command #{} LOGIC_RESET", LCB::LOGIC_RESET);
    sendCmd(LCB::fast_command(LCB::LOGIC_RESET, 0) );

    // Reset HCCs
    resetHCCStars();

    // Minimal configurations for HCCs to establish communications with ABCs(v0)
    logger->debug("Configure HCCs to establish communications with ABCs");
    //
    // Broadcast the register commands without modifying hccregisterMap
    // Delay 1 (register 32): delay for signals from HCC to ABCs
    uint32_t val_hcc32 = 0x02400000;
    logger->trace("Writing 0x{:08x} to Register {} of all HCCs", val_hcc32, 32);
    sendCmd(write_hcc_register(32, val_hcc32));
    //
    // DRV1 (register 38): enable driver and currents
    uint32_t val_hcc38 = 0x0fffffff;
    logger->trace("Writing 0x{:08x} to Register {} of all HCCs", val_hcc38, 38);
    sendCmd(write_hcc_register(38, val_hcc38) );

    // Reset ABCs
    resetABCStars();

    // Star PR&LP to ABCs
    logger->debug("Sending fast command #{} HCC_START_PRLP", LCB::HCC_START_PRLP);
	sendCmd(LCB::fast_command(LCB::HCC_START_PRLP, 0) );
}

void StarChips::configure() {

	//Set the HCC ID
        if (m_fuse_id) this->setHccId(getHCCchipID());

	logger->info("Sending registers configuration...");

	this->writeRegisters();

    logger->debug("Sending fast command #{} LOGIC_RESET", LCB::LOGIC_RESET);
    sendCmd(LCB::fast_command(LCB::LOGIC_RESET, 0) );

    logger->debug("Sending lonely_BCR");
    sendCmd(LCB::lonely_bcr());

    // Make histo size match number of configured ABCs
    geo.nCol = 128 * numABCs();
}

void StarChips::sendCmd(uint16_t cmd){
	//	std::cout << std::hex <<cmd << std::dec<< "_"<<std::endl;

	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->writeFifo((cmd << 16) + LCB::IDLE);
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->releaseFifo();

}

void StarChips::sendCmd(std::array<uint16_t, 9> cmd){
	//    std::cout << __PRETTY_FUNCTION__ << "  txChannel: " << getTxChannel() << " cmd:  " <<  cmd << std::endl;
	//	for( auto a : cmd ) {
	//		std::cout << std::hex <<a << std::dec<< "_";
	//	}
	//	std::cout <<  std::endl;

	//	std::cout << std::hex <<((cmd[0] << 16) + cmd[1])<< std::dec<< "_";
	//	std::cout << std::hex <<((cmd[2] << 16) + cmd[3]) << std::dec<< "_";
	//	std::cout << std::hex <<((cmd[4] << 16) + cmd[5])<< std::dec<< "_";
	//	std::cout << std::hex <<((cmd[6] << 16) + cmd[7])<< std::dec<< "_";
	//	std::cout << std::hex <<((cmd[8] << 16) + 0)<< std::dec<< "_";
	//	std::cout <<  std::endl;
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->writeFifo((cmd[0] << 16) + cmd[1]);
	m_txcore->writeFifo((cmd[2] << 16) + cmd[3]);
	m_txcore->writeFifo((cmd[4] << 16) + cmd[5]);
	m_txcore->writeFifo((cmd[6] << 16) + cmd[7]);
	m_txcore->writeFifo((cmd[8] << 16) + LCB::IDLE);
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->writeFifo((LCB::IDLE << 16) + LCB::IDLE);
	m_txcore->releaseFifo();

}

bool StarChips::writeTrims(){
    //Write only TrimDAC registers so we don't overwrite the prescan when doing a trim
    auto num_abc = numABCs();
    int hccId = getHCCchipID();

    // Then each ABC
    eachAbc([&](auto &abc) {
            int this_chipID = abc.getABCchipID();

            logger->info("Write ABC {} trim registers", this_chipID);
            for(unsigned int addr = ABCStarRegister::TrimDAC0; addr <= ABCStarRegister::TrimDAC39; addr++) {
                logger->debug("Writing Register {} for chipID {}", addr, this_chipID);
                writeABCRegister(addr, abc);
            }
            logger->info("Done with ABC {}", this_chipID);
        });

    return true;
    
}

bool StarChips::writeRegisters(){
	//Write all register to their setting, both for HCC & all ABCs
        auto num_abc = numABCs();
	logger->info("Write registers for HCC + {} * ABCs", num_abc);

        // First write HCC
        int hccId = getHCCchipID();

        const auto &hcc_regs = m_hcc_info->hccWriteMap;
	logger->info("Starting on HCC {} with {} registers", hccId, hcc_regs.size());

        for(auto &map_iter: hcc_regs) {
              auto addr = map_iter.first;
              logger->trace("Writing HCC Register {} for chipID {}", addr, hccId);
              writeHCCRegister(addr);
        }

        // Then each ABC
        const auto &abc_regs = m_abc_info->abcWriteMap;
	eachAbc([&](auto &abc) {
                int this_chipID = abc.getABCchipID();

                logger->info("Starting on ABC {} with {} registers", this_chipID, abc_regs.size());
		for(auto &map_iter: abc_regs) {
                        auto addr = map_iter.first;
                        logger->debug("Writing Register {} for chipID {}", addr, this_chipID);

                        writeABCRegister(addr, abc);
		}
		logger->info("Done with ABC {}", this_chipID);
          });

	return true;
}

//Will write value for setting name for the HCC if name starts with "HCC_" otherwise will write the setting for all ABCs if name starts with "ABCs_"
void StarChips::writeNamedRegister(std::string name, uint16_t reg_value) {
  std::string strPrefix = name.substr (0,4);
  //if we deal with a setting for the HCC, look up in register map.
  if (strPrefix=="HCC_") {
    auto subRegName = name.substr(4);
    if(!HCCStarSubRegister::_is_valid(subRegName.c_str())) {
      logger->error(" --> Error: Could not find HCC sub-register \"{}\"", subRegName);
    } else {
      setAndWriteHCCSubRegister(subRegName, reg_value);
    }
  } else  if (strPrefix=="ABCs") {
    auto subRegName = name.substr(5); // Including _
    if(subRegName == "MASKs") {
      // Special case for digitial scan
      uint32_t val = (reg_value == 0)?0:0xffffffff;
      logger->trace("Writing {:08x} to mask register for all ABCStar chips.", val);
      eachAbc([&](auto &cfg) {
          for(int m = ABCStarRegister::MaskInput(0);
              m <= ABCStarRegister::MaskInput(7); m++) {
            cfg.setRegisterValue(ABCStarRegister::_from_integral(m), val);
            sendCmd( write_abc_register(m, val,
                                        getHCCchipID(), cfg.getABCchipID()));
          }
        });
    } else if(!ABCStarSubRegister::_is_valid(subRegName.c_str())) {
      logger->error(" --> Error: Could not find ABC sub-register \"{}\"", subRegName);
    } else {
      logger->trace("Writing {} on setting '{}' for all ABCStar chips.", reg_value, name);
      eachAbc([&](auto &cfg) {
          setAndWriteABCSubRegister(subRegName, cfg, reg_value);
        });
    }
  }
}


void StarChips::readRegisters(){

	//Read all known registers, both for HCC & all ABCs
        logger->debug("Looping over all chips in readRegisters, where m_nABC is {}", numABCs());

        auto &hcc_regs = m_hcc_info->hccregisterMap;

        for(auto &map_iter: hcc_regs) {
                auto addr = map_iter.first;
                // Skip HCCCommand reg
                if(addr == 16) continue;
                int this_chipID = getHCCchipID();
                logger->debug("Calling readRegister for HCC {} register {}", this_chipID, addr);
                readHCCRegister(addr);
        }

        const auto &abc_regs = m_abc_info->abcregisterMap;

        eachAbc([&](const auto &abc) {
                int this_chipID = abc.getABCchipID();
                for(auto &map_iter: abc_regs) {
                        auto addr = map_iter.first;

                        logger->debug("Hcc id: {}", getHCCchipID());
                        logger->debug("Calling readRegister for chipID {} register {}", this_chipID, addr);

                        readABCRegister(addr, this_chipID);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        logger->debug("Not calling read()");
                        //                      read(map_iter->first, rxcore);
                }//for each register address
          }); //for each chipID

}

void StarChips::writeHCCRegister(int addr) {
    uint32_t value = m_hcc.getRegisterValue(HCCStarRegister::_from_integral(addr));
    logger->debug("Doing HCC write register with value 0x{:08x} from registerMap[addr={}]", value, addr);
    sendCmd(write_hcc_register(addr, value, getHCCchipID()));
}

void StarChips::writeABCRegister(int addr, AbcCfg &cfg) {
    uint32_t value = cfg.getRegisterValue(ABCStarRegister::_from_integral(addr));
    auto id = cfg.getABCchipID();
    logger->debug("Doing ABC ID {} writeRegister {} with value 0x{:08x}", id, addr, value);
    sendCmd(write_abc_register(addr, value, getHCCchipID(), id));
}

void StarChips::readHCCRegister(int addr) {
    sendCmd(read_hcc_register(addr, getHCCchipID()));
}

void StarChips::readABCRegister(int addr, int32_t chipID) {
    sendCmd(read_abc_register(addr, getHCCchipID(), chipID));
}
