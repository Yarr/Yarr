#include "StarChipsetEmu.h"

#include <iomanip>

#include "ScanHelper.h"
#include "logging.h"

namespace {

template<typename T>
struct print_hex_type {
  T v;
};

template<typename T>
print_hex_type<T> print_hex(T val) {
  return {val};
}

template<typename T>
std::ostream &operator <<(std::ostream &os, print_hex_type<T> v) {
  // Width in nibbles
  int w = sizeof(T) * 2;
  os << std::hex << "0x" << std::setw(w) << std::setfill('0')
     << static_cast<unsigned int>(v.v) << std::dec << std::setfill(' ');
  return os;
}

auto logger = logging::make_log("StarChipsetEmu");
}

StarChipsetEmu::StarChipsetEmu(ClipBoard<RawData>* rx,
                               const std::string& json_emu_file_path,
                               std::unique_ptr<StarCfg> regCfg,
                               unsigned hpr_period, int abc_version, int hcc_version)
  : m_rxbuffer ( rx )
  , HPRPERIOD( hpr_period )
  , m_abc_version( abc_version )
  , m_hcc_version( hcc_version )
  , m_starCfg (std::move(regCfg))
{
  // Emulator analog FE configurations
  if (not json_emu_file_path.empty()) {
    json jEmu;
    try {
      jEmu = ScanHelper::openJsonFile(json_emu_file_path);
    } catch (std::runtime_error &e) {
      logger->error("Error opening emulator config: {}", e.what());
      throw(std::runtime_error("StarChipsetEmu::StarChipsetEmu failure"));
    }
    // Initialize FE strip array from config json
    for (size_t istrip = 0; istrip < 256; ++istrip) {
      m_stripArray[istrip].setValue(jEmu["vthreshold_mean"][istrip],
                                    jEmu["vthreshold_sigma"][istrip],
                                    jEmu["noise_occupancy_mean"][istrip],
                                    jEmu["noise_occupancy_sigma"][istrip]);
    }
  }

  // HPR
  hpr_clkcnt = HPRPERIOD/2; // 20000 BCs or 500 us
  hpr_sent.resize(m_starCfg->numABCs() + 1); // 1 HCCStar + nABCs ABCStar chips
  std::fill(hpr_sent.begin(), hpr_sent.end(), false);
}

StarChipsetEmu::~StarChipsetEmu() = default;

void StarChipsetEmu::sendPacket(uint8_t *byte_s, uint8_t *byte_e) {
    size_t byte_length = byte_e - byte_s;

    size_t word_length = (byte_length + 3) / 4;

    std::unique_ptr<RawData> data(new RawData(0, word_length));
    uint32_t *buf = data->getBuf();

    for(unsigned i=0; i<byte_length/4; i++) {
        buf[i] = *(uint32_t*)&byte_s[i*4];
    }

    if(byte_length%4) {
        uint32_t final = 0;
        for(unsigned i=0; i<byte_length%4; i++) {
            int offset = 8 * (i);
            final |= byte_s[(word_length-1)*4 + i] << offset;
        }
        buf[word_length-1] = final;
    }

    m_rxbuffer->pushData(std::move(data));
}

//
// Build data packets
//
bool StarChipsetEmu::getParity_8bits(uint8_t val) {
  val ^= val >> 4;
  val ^= val >> 2;
  val ^= val >> 1;
  return val&1;
}

std::vector<uint8_t> StarChipsetEmu::buildPhysicsPacket(
  const std::vector<std::vector<uint16_t>>& allClusters,
  PacketTypes typ, uint8_t l0tag, uint8_t bc_count, uint16_t endOfPacket) {
  std::vector<uint8_t> data_packets;

  ///////////////////
  // Header: 16 bits
  bool errorflag = 0; // for now
  // BCID: lowest 3 bits of 8-bit  + 1 parity bit 
  bool bc_parity = getParity_8bits(bc_count);
  // packet type (4b) + flag error (1b) + L0tag (7b) + BCID (4b)
  uint16_t header = ((uint8_t)typ << 12) | errorflag << 11 | (l0tag & 0x7f) << 4 | (bc_count&7) << 1 | bc_parity;

  data_packets.push_back((header>>8) & 0xff);
  data_packets.push_back(header & 0xff);

  ///////////////////
  // ABCStar clusters
  for (size_t ichannel=0; ichannel<allClusters.size(); ++ichannel) {
    for ( uint16_t cluster : allClusters[ichannel]) {
      // cluster bits:
      // "0" + 4-bit channel number + 11-bit cluster dropping the last cluster bit
      uint16_t clusterbits = (ichannel & 0xf)<<11 | (cluster & 0x7ff);
      data_packets.push_back((clusterbits>>8) & 0xff);
      data_packets.push_back(clusterbits & 0xff);
    }
  }

  // Todo: error block

  if (data_packets.size() == 2) {
    // Only the two header bytes. No clusters added from any ABCStars.
    // return an empty vector;
    data_packets.clear();
  } else {
    // Add the end of packet pattern
    data_packets.push_back((endOfPacket>>8) & 0xff);
    data_packets.push_back(endOfPacket & 0xff);
  }

  return data_packets;
}

std::vector<uint8_t> StarChipsetEmu::buildABCRegisterPacket(
  PacketTypes typ, uint8_t input_channel, uint8_t reg_addr, unsigned reg_data,
  uint16_t reg_status) {

  std::vector<uint8_t> data_packets;

  // first byte: 4-bit type + 4-bit HCC input channel
  uint8_t byte1 = ((uint8_t)typ & 0xf ) << 4 | (input_channel & 0xf);
  data_packets.push_back(byte1);

  // then 8-bit register address
  data_packets.push_back(reg_addr);

  // 4-bit TBD + 32-bit data + 16-bit status + '0000'
  data_packets.push_back(reg_data >> 28);
  data_packets.push_back((reg_data >> 20) & 0xff);
  data_packets.push_back((reg_data >> 12) & 0xff);
  data_packets.push_back((reg_data >> 4) & 0xff);
  data_packets.push_back((reg_data & 0xf) << 4 | ((reg_status >> 12) & 0xf));
  data_packets.push_back((reg_status >> 4) & 0xff);
  data_packets.push_back((reg_status & 0xf) << 4);

  return data_packets;
}

std::vector<uint8_t> StarChipsetEmu::buildHCCRegisterPacket(
  PacketTypes typ, uint8_t reg_addr, unsigned reg_data) {

  std::vector<uint8_t> data_packets;
  
  // 4-bit type + 8-bit register address + 32-bit data + '0000'
  data_packets.push_back( ((uint8_t)typ & 0xf) << 4 | (reg_addr >> 4) );
  data_packets.push_back( ((reg_addr & 0xf) << 4) | (reg_data >> 28) );
  data_packets.push_back((reg_data >> 20) & 0xff);
  data_packets.push_back((reg_data >> 12) & 0xff);
  data_packets.push_back((reg_data >> 4) & 0xff);
  data_packets.push_back((reg_data & 0xf) << 4);

  return data_packets;
}

//
// Register commands
//
void StarChipsetEmu::doRegReadWrite(LCB::Frame frame) {
  uint8_t code0 = (frame >> 8) & 0xff;
  uint8_t code1 = frame & 0xff;

  SPDLOG_LOGGER_TRACE(logger, "Receive a register command -> symbol1 = 0x{:x}, symbol2 = 0x{:x}", code0, code1);

  if (code0 == LCB::K2) { // This frame is a K2 Start or K2 End

    // Decode the second symbol
    uint8_t data6 = SixEight::decode(code1);
    bool isK2Start = (data6 >> 4) & 1; // Otherwise it is a K2 End
    unsigned cmd_hccID = data6 & 0xf; // Bottom 4 bits for HCC ID
    // Ignore the command sequence unless the HCC ID matches the ID on chip
    // or it is a broadcast command (0b1111)
    m_ignoreCmd = not ( cmd_hccID == (m_starCfg->getHCCchipID() & 0xf) or cmd_hccID == 0xf);

    if (m_ignoreCmd) return;
    
    if (isK2Start) {
      m_isForABC = (data6 >> 5) & 1; // Otherwise it is a HCC command

      // Clear the command buffer if it is not empty
      // (in case a second K2 Start is received before a K2 End)
      if (not m_reg_cmd_buffer.empty()) {
        std::queue<uint8_t> empty_buffer;
        std::swap(m_reg_cmd_buffer, empty_buffer);
      }
    }
    else { // K2 End
      size_t bufsize = m_reg_cmd_buffer.size();
      if ( not (bufsize==2 or bufsize==7) ) {
        // If K2 End occurs at the wrong stage, no action is taken.
        logger->warn("K2 End received at the wrong position! Current command sequence size (excluding K2 frames): {}", m_reg_cmd_buffer.size());
        return;
      }

      execute_command_sequence();
    } // if (isK2Start)
  } else { // not K2 Start or End
    if (m_ignoreCmd) return;

    // Decode the frame
    uint16_t data12 = (SixEight::decode(code0) << 6) | (SixEight::decode(code1));
    // Top 5 bits should be zeros
    assert(not (data12>>7));
    // Store the lowest 7 bits into the buffer.
    m_reg_cmd_buffer.push(data12 & 0x7f);
  }
}

void StarChipsetEmu::writeRegister(const uint32_t data, const uint8_t address,
                                   bool isABC, const unsigned ABCID) {
  if (isABC) {
    // skip writing if the register is read only
    if (address == ABCStarRegister::STAT0 or
        address == ABCStarRegister::STAT1 or
        address == ABCStarRegister::STAT2 or
        address == ABCStarRegister::STAT3 or
        address == ABCStarRegister::STAT4 or
        address == ABCStarRegister::HPR
        or (address >= ABCStarRegister::Counter(0) && address <= ABCStarRegister::Counter(63))) {
      logger->warn("A register write command is received for a read-only ABCStar register 0x{:x}. Skip writing.", address);
      return;
    } else {
      try {
        m_starCfg->setABCRegister(address, data, ABCID);
      } catch (std::out_of_range &e) {
        logger->warn("Unexpected out of range for register write of ABCStar register 0x{:x}. Skip writing.", address);
        return;
      }
    }
  } else {
    // skip writing if the register is read only
    if (address == HCCStarRegister::SEU1 or
        address == HCCStarRegister::SEU2 or
        address == HCCStarRegister::SEU3 or
        address == HCCStarRegister::FrameRaw or
        address == HCCStarRegister::LCBerr or
        address == HCCStarRegister::ADCStatus or
        address == HCCStarRegister::Status or
        address == HCCStarRegister::HPR) {
      logger->warn("A register write command is received for a read-only HCCStar register 0x{:x}. Skip writing.", address);
      return;
    } else if (address == HCCStarRegister::Addressing) {
      // special case for dynamic addressing
      // only the top 4 bits are read-write bits and are used as HCC ID
      uint32_t hccid_cur = m_starCfg->getHCCRegister(HCCStarRegister::Addressing);
      if ((data & 0xfff) == (hccid_cur & 0xfff)) {
        // update the top 4 bits and the HCC ID only if the bottom 24
        // bits from the write command match the bottom 24 bits of the
        // Addressing register
        uint32_t hccid_new = (data & 0xf000) | (hccid_cur & 0x0fff);
        m_starCfg->setHCCRegister(address, hccid_new);
        m_starCfg->setHCCChipId(data>>24);
      }
    } else {
      try {
        m_starCfg->setHCCRegister(address, data);
      } catch (std::out_of_range &e) {
        logger->warn("Unexpected out of range for register write of HCCStar register 0x{:x}. Skip writing.", address);
        return;
      }
    }
  }
}

void StarChipsetEmu::readRegister(const uint8_t address, bool isABC,
                                  const unsigned ABCID) {
  if (isABC) { // Read ABCStar registers
    PacketTypes ptype = PacketTypes::ABCRegRd;

    // HCCStar channel number
    unsigned ich = m_starCfg->hccChannelForABCchipID(ABCID);
    if (ich >= m_starCfg->numABCs()) {
      logger->warn("Cannot find an ABCStar chip with ID = {}", ABCID);
      return;
    }

    // read register
    unsigned data;
    try {
      data = m_starCfg->getABCRegister(address, ABCID);
    } catch(std::out_of_range &e) {
      // non-existant register
      data = 0xffffffff;
    }

    // ABC status bits
    // for now
    uint16_t status = (ABCID & 0xf) << 12;
    /*
    status[15:0] = {ABCID[3:0], 0, BCIDFlag,
                    PRFIFOFull, PRFIFOEmpty, LPFIFOFull, LPFIFOEmpty,
                    RegFIFOOVFL, RegFIFOFull, RegFIFOEmpty,
                    ClusterOVFL, ClusterFull, ClusterEmpty};
    */

    // build and send data packet
    auto packet = buildABCRegisterPacket(ptype, ich, address, data, status);
    sendPacket(packet);
  } else { // Read HCCStar registers
    PacketTypes ptype = PacketTypes::HCCRegRd;

    // read register
    unsigned data;
    try {
      data = m_starCfg->getHCCRegister(address);
    } catch(std::out_of_range &e) {
      // non-existant register
      data = 0xffffffff;
    }

    // build and send data packet
    auto packet = buildHCCRegisterPacket(ptype, address, data);
    sendPacket(packet);
  }
}

void StarChipsetEmu::execute_command_sequence() {
  // Obtain and parse the header
  uint8_t header1 = m_reg_cmd_buffer.front();
  m_reg_cmd_buffer.pop();
  uint8_t header2 = m_reg_cmd_buffer.front();
  m_reg_cmd_buffer.pop();

  bool isRegRead = (header1 >> 6) & 1; // Otherwise write register
  unsigned cmd_abcID = (header1 >> 2) & 0xf;
  uint8_t reg_addr = ((header1 & 3) << 6) | ((header2 >> 1) & 0x3f);

  // Access register
  if (isRegRead) { // register read command
    logger->debug("Receive a register read command -> addr = 0x{:x} isABC = {} abcID = {}", reg_addr, m_isForABC, cmd_abcID);
    /*    
    if (not m_reg_cmd_buffer.empty()) {
      logger->warn("Command sequence is of wrong size for a register read!");
      return;
    }
    */

    // If cmd_abcID is '1111' i.e. broadcast address, read all ABCs
    if ((cmd_abcID & 0xf) == 0xf and m_isForABC) {
      for (size_t index=1; index <= m_starCfg->numABCs(); ++index)
        readRegister(reg_addr, true, m_starCfg->getABCchipID(index));
    } else {
      readRegister(reg_addr, m_isForABC, cmd_abcID);
    }
  } else { // register write command
    if (m_reg_cmd_buffer.size() != 5) {
      logger->warn("Command sequence is of wrong size for a register write!");
      return;
    }

    uint32_t data = 0;
    for (int i = 4; i >= 0; --i) {
      data |= ((m_reg_cmd_buffer.front() & 0x7f) << (7*i));
      m_reg_cmd_buffer.pop();
    }

    logger->debug("Receive a register write command -> addr = 0x{:x} data = 0x{:x} isABC = {} abcID = {}", (int)reg_addr, data, m_isForABC, cmd_abcID);

    // write register
    // If cmd_abcID is '1111' i.e. broadcast address, write all ABCs
    if ((cmd_abcID & 0xf) == 0xf and m_isForABC) {
      for (int index=1; index <= m_starCfg->numABCs(); ++index)
        writeRegister(data, reg_addr, true, m_starCfg->getABCchipID(index));
    } else {
      writeRegister(data, reg_addr, m_isForABC, cmd_abcID);
    }
    assert(m_reg_cmd_buffer.empty());
  } // if (isRegRead)
}

//
// Fast commands
//
void StarChipsetEmu::doFastCommand(uint8_t data6) {
  uint8_t bcsel = (data6 >> 4) & 3; // top 2 bits for BC select
  m_bc_sel = bcsel;
  uint8_t fastcmd = data6 & 0xf; // bottom 4 bits for command

  logger->debug("Receive a fast command #{} (BC select = {})", fastcmd, bcsel);

  // Reset commands reset everything at once, ignoring the selected BC for now
  switch(fastcmd) {
  case LCB::LOGIC_RESET :
    this->logicReset();
    break;
  case LCB::ABC_REG_RESET :
    this->resetABCRegisters();
    break;
  case LCB::ABC_SEU_RESET :
    this->resetABCSEU();
    break;
  case LCB::ABC_CAL_PULSE :
    this->ackPulseCmd(1, bcsel);
    break;
  case LCB::ABC_DIGITAL_PULSE :
    this->ackPulseCmd(2, bcsel);
    break;
  case LCB::ABC_HIT_COUNT_RESET :
    this->resetABCHitCounts();
    break;
  case LCB::ABC_HIT_COUNT_START :
    m_startHitCount = true;
    break;
  case LCB::ABC_HIT_COUNT_STOP :
    m_startHitCount = false;
    break;
  case LCB::ABC_SLOW_COMMAND_RESET :
    this->resetSlowCommand();
    break;
  case LCB::HCC_STOP_PRLP :
    //std::cout << "Fast command: StopPRLP" << std::endl;
    break;
  case LCB::HCC_REG_RESET :
    this->resetHCCRegisters();
    //hpr_clkcnt = HPRPERIOD/2;
    //std::fill(hpr_sent.begin(), hpr_sent.end(), false);
    break;
  case LCB::HCC_SEU_RESET :
    this->resetHCCSEU();
    break;
  case LCB::HCC_PLL_RESET :
    this->resetHCCPLL();
    break;
  case LCB::HCC_START_PRLP :
    //std::cout << "Fast command: StartPRLP" << std::endl;
    break;
  }
}

void StarChipsetEmu::logicReset() {
  hpr_clkcnt = HPRPERIOD/2;
  std::fill(hpr_sent.begin(), hpr_sent.end(), false);

  (m_starCfg->hcc()).setSubRegisterValue("TESTHPR", 0);
  (m_starCfg->hcc()).setSubRegisterValue("STOPHPR", 0);
  (m_starCfg->hcc()).setSubRegisterValue("MASKHPR", 0);

  m_starCfg->eachAbc([&](auto &abc) {
      abc.setSubRegisterValue("TESTHPR", 0);
      abc.setSubRegisterValue("STOPHPR", 0);
      abc.setSubRegisterValue("MASKHPR", 0);
    });

  clearFEData();
}

void StarChipsetEmu::resetABCRegisters() {
  m_starCfg->eachAbc([&](auto &abc){abc.setDefaults(m_abc_version);});
  resetABCHitCounts();
}

void StarChipsetEmu::resetABCSEU() {
  m_starCfg->eachAbc([&](auto &abc) {
      abc.setRegisterValue(ABCStarRegister::STAT0, 0x00000000);
      abc.setRegisterValue(ABCStarRegister::STAT1, 0x00000000);
    });
}

void StarChipsetEmu::resetABCHitCounts() {
  m_starCfg->eachAbc([&](auto &abc) {
      for (unsigned int iReg=ABCStarRegister::HitCountREG0; iReg<=ABCStarRegister::HitCountREG63; iReg++) {
        abc.setRegisterValue(ABCStarRegister::_from_integral(iReg), 0x00000000);
      }
    });
}

void StarChipsetEmu::resetSlowCommand() {
  m_ignoreCmd = true;
  m_isForABC = false;

  // clear command buffer
  std::queue<uint8_t> empty_buffer;
  std::swap(m_reg_cmd_buffer, empty_buffer);
}

void StarChipsetEmu::resetHCCRegisters() {
  (m_starCfg->hcc()).setDefaults(m_hcc_version);
}

void StarChipsetEmu::resetHCCSEU() {
  m_starCfg->setHCCRegister(HCCStarRegister::SEU1, 0x00000000);
  m_starCfg->setHCCRegister(HCCStarRegister::SEU2, 0x00000000);
  m_starCfg->setHCCRegister(HCCStarRegister::SEU3, 0x00000000);
}

void StarChipsetEmu::resetHCCPLL() {
  m_starCfg->setHCCRegister(HCCStarRegister::PLL1, 0x00000000);
  m_starCfg->setHCCRegister(HCCStarRegister::PLL2, 0x00000000);
  m_starCfg->setHCCRegister(HCCStarRegister::PLL3, 0x00000000);
}

//
// HPR
//
void StarChipsetEmu::doHPR(LCB::Frame frame) {
  doHPR_HCC(frame);

  for (unsigned ichip = 1; ichip <= m_starCfg->numABCs(); ++ichip) {
    doHPR_ABC(frame, ichip);
  }

  // Each LCB command frame covers 4 BCs
  hpr_clkcnt += 4;
}

void StarChipsetEmu::doHPR_HCC(LCB::Frame frame) {
  //// Update the HPR register
  setHCCStarHPR(frame);

  //// HPR control logic
  bool testHPR = m_starCfg->getSubRegisterValue(0, "TESTHPR");
  bool stopHPR = m_starCfg->getSubRegisterValue(0, "STOPHPR");
  bool maskHPR = m_starCfg->getSubRegisterValue(0, "MASKHPR");

  // Assume for now in the software emulation LCB is always locked and only
  // testHPR bit can trigger the one-time pulse to send an HPR packet
  // The one-time pulse is ignored if maskHPR is one.
  bool lcb_lock_changed = testHPR & (!maskHPR);

  // An HPR packet is also sent periodically:
  // 500 us (20000 BCs) after reset and then every 1 ms (40000 BCs)
  // (hpr_clkcnt is initialized to 20000 BCs)
  bool hpr_periodic = not (hpr_clkcnt%HPRPERIOD) and not stopHPR;

  // Special cases for the initial HPR packet after a reset
  // If stopHPR is set to 1 before any HPR packet is sent, send one immediately
  bool hpr_initial = not hpr_sent[0] and stopHPR;

  //// Build and send the HPR packet
  if (lcb_lock_changed or hpr_periodic or hpr_initial) {
    auto packet_hcchpr = buildHCCRegisterPacket(
      PacketTypes::HCCHPR, (+HCCStarRegister::HPR)._to_integral(),
      m_starCfg->getHCCRegister(HCCStarRegister::HPR));

    sendPacket(packet_hcchpr);

    hpr_sent[0] = true;
  }

  //// Update HPR control bits
  // Reset stopHPR to zero (i.e. resume the periodic HPR packet transmission)
  // if lcb_lock_changed
  if (stopHPR and lcb_lock_changed)
    m_starCfg->setSubRegisterValue(0, "STOPHPR", 0);

  // Reset testHPR bit to zero if it is one
  if (testHPR)
    m_starCfg->setSubRegisterValue(0, "TESTHPR", 0);
}

void StarChipsetEmu::doHPR_ABC(LCB::Frame frame, unsigned ichip) {
  int abcID = m_starCfg->getABCchipID(ichip);

  //// Update the HPR register
  setABCStarHPR(frame, abcID);

  //// HPR control logic
  bool testHPR = m_starCfg->getSubRegisterValue(ichip, "TESTHPR");
  bool stopHPR = m_starCfg->getSubRegisterValue(ichip, "STOPHPR");
  bool maskHPR = m_starCfg->getSubRegisterValue(ichip, "MASKHPR");

  bool lcb_lock_changed = testHPR & (!maskHPR);
  bool hpr_periodic = not (hpr_clkcnt%HPRPERIOD) and not stopHPR;
  bool hpr_initial = not hpr_sent[ichip] and stopHPR;

  //// Build and send HPR packets
  if (lcb_lock_changed or hpr_periodic or hpr_initial) {
    auto packet_abchpr = buildABCRegisterPacket(
      PacketTypes::ABCHPR, ichip-1, (+ABCStarRegister::HPR)._to_integral(),
      m_starCfg->getABCRegister(ABCStarRegister::HPR, abcID), (abcID&0xf) << 12);

    sendPacket(packet_abchpr);

    hpr_sent[ichip] = true;
  }

  //// Update HPR control bits
  if (stopHPR and lcb_lock_changed)
    m_starCfg->setSubRegisterValue(ichip, "STOPHPR", 0);
  if (testHPR)
    m_starCfg->setSubRegisterValue(ichip, "TESTHPR", 0);
}

void StarChipsetEmu::setHCCStarHPR(LCB::Frame frame) {
  // Dummy status bits
  bool R3L1_errcount_ovfl = 0;
  bool ePllInstantLock = 1;
  bool lcb_scmd_err = 0;
  bool lcb_errcount_ovfl = 1;
  bool lcb_decode_err = 0;
  bool lcb_locked = 1;
  bool R3L1_locked = 1;

  uint32_t hprWord = frame << 16 |
    R3L1_errcount_ovfl << 6 | ePllInstantLock << 5 | lcb_scmd_err << 4 |
    lcb_errcount_ovfl << 3 | lcb_decode_err << 2 | lcb_locked << 1 |
    R3L1_locked;

  m_starCfg->setHCCRegister(HCCStarRegister::HPR, hprWord);
}

void StarChipsetEmu::setABCStarHPR(LCB::Frame frame, int abcID) {
  // Dummy status bits
  bool LCB_SCmd_Err = 0;
  bool LCB_ErrCnt_Ovfl = 1;
  bool LCB_Decode_Err = 0;
  bool LCB_Locked = 1;
  uint16_t ADC_dat = 0xfff;

  uint32_t hprWord = frame << 16 |
    LCB_SCmd_Err << 15 | LCB_ErrCnt_Ovfl << 14 | LCB_Decode_Err << 13 |
    LCB_Locked << 12 | ADC_dat;

  m_starCfg->setABCRegister(ABCStarRegister::HPR, hprWord, abcID);
}

//
// Trigger and front-end data
//
void StarChipsetEmu::doL0A(bool bcr, uint8_t l0a_mask, uint8_t l0a_tag) {
  logger->debug("Receive an L0A command: BCR = {}, L0A mask = {:b}, L0A tag = 0x{:x}", bcr, l0a_mask, l0a_tag);

  bool trig_mode = m_starCfg->getSubRegisterValue(0, "TRIGMODE"); // TRIGMODEC?
  logger->debug("Trigger mode is {}", trig_mode ? "single-level" : "multi-level");

  // An LCB frame covers 4 BCs
  for (unsigned ibc = 0; ibc < 4; ++ibc) {
    // check if there is an L0A
    // msb of the L0A mask corresponds to the earliest BC
    if (not ((l0a_mask >> (3-ibc)) & 1) ) continue;

    if (trig_mode) { // single-level trigger
      // clusters
      std::vector<std::vector<uint16_t>> clusters;
      uint8_t bcid;

      // for each ABC
      m_starCfg->eachAbc([this, ibc, &bcid, &clusters](auto& abc) {
          // L0 address
          auto l0addr = this->getL0BufferAddr(abc, ibc);

          StripData hits;
          std::tie(bcid, hits) = this->getFEData(abc, l0addr);

          // count hits
          this->countHits(abc, hits);

          // form clusters
          if (abc.getSubRegisterValue("LP_ENABLE")) {
            auto abc_clusters = this->getClusters(abc, hits);
            clusters.push_back(abc_clusters);
          }
        });

      // build and send data packet
      PacketTypes ptype = PacketTypes::LP;
      std::vector<uint8_t> packet = buildPhysicsPacket(clusters, ptype, l0a_tag+ibc, bcid);
      if (not packet.empty()) {
        sendPacket(packet);
      }

    } else { // multi-level trigger
      // for each ABC
      m_starCfg->eachAbc([this, ibc, l0a_tag](auto& abc) {
          // L0 address
          auto l0addr = this->getL0BufferAddr(abc, ibc);

          // fill m_evtbuffers_lite
          int abcId = abc.getABCchipID();
          if (m_evtbuffers_lite.find(abcId) == m_evtbuffers_lite.end())
            m_evtbuffers_lite[abcId] = std::array<EvtBufData, EvtBufDepth>();

          // event buffer address
          uint8_t evaddr = (l0a_tag+ibc)%EvtBufDepth;
          // 8-bit BCID@L0A
          auto bcidl0 = EvtBufData( (m_bccnt+ibc) & 0xff );
          m_evtbuffers_lite[abcId][evaddr] = EvtBufData(l0addr)<<8 | bcidl0;

          // count hits
          StripData hits = this->getFEData(abc, l0addr).second;
          this->countHits(abc, hits);
        });
    }
  } // 4 BCs

  //if (bcr) m_resetbc = true;
}

void StarChipsetEmu::doPRLP(uint8_t mask, uint8_t l0tag) {
  bool trig_mode = m_starCfg->getSubRegisterValue(0, "TRIGMODE"); // TRIGMODEC?
  if (trig_mode) { // single-level
    logger->critical("doPRLP is called while the trigger mode is single level");
    return;
  }

  bool isPR;
  if (mask) { // a R3 command
    isPR = true;
    // module number
    unsigned module = (m_starCfg->getHCCchipID())/2;
    if ( not (module>=1 and module <=5 and ((mask>>(module-1))&1)) ) {
      logger->debug("Skip R3 command with mask {:b} for HCCStar {}", mask, m_starCfg->getHCCchipID());
      return;
    }
  } else { // an L1 command
    isPR = false;
  }

  // clusters
  std::vector<std::vector<uint16_t>> clusters;
  uint8_t bcid;

  // for each ABC
  m_starCfg->eachAbc([this, l0tag, isPR, &bcid, &clusters](auto& abc) {
      if (isPR and not abc.getSubRegisterValue("PR_ENABLE")) {
        // Skip if a R3 command for PR packets is received but PR_ENABLE is 0
        return;
      }

      if (not isPR and not abc.getSubRegisterValue("LP_ENABLE")) {
        // Skip if an L1 command for LP packets is received but LP_ENABLE is 0
        return;
      }

      int abcId = abc.getABCchipID();
      if (m_evtbuffers_lite.find(abcId) == m_evtbuffers_lite.end()) {
        logger->critical("No event buffer instantiated for chip ID {}", abcId);
      }

      // access event buffer via l0tag
      auto evtdata = m_evtbuffers_lite[abcId][l0tag];
      // bottom 8 bits are BCID@L0A
      uint8_t bcl0 = evtdata.to_ulong() & 0xff;
      // top 9 bits are L0 buffer address
      uint16_t l0addr = (evtdata>>8).to_ulong();

      // get FE hits and form clusters
      StripData hits;
      std::tie(bcid, hits) = this->getFEData(abc, l0addr);
      auto abc_clusters = this->getClusters(abc, hits);
      clusters.push_back(abc_clusters);
    });

  // build and send data packet
  PacketTypes ptype = isPR ? PacketTypes::PR : PacketTypes::LP;
  std::vector<uint8_t> packet = buildPhysicsPacket(clusters, ptype, l0tag, bcid);
  if (not packet.empty()) {
    sendPacket(packet);
  }
}

unsigned int StarChipsetEmu::countTriggers(LCB::Frame frame) {
  uint8_t code0 = (frame >> 8) & 0xff;
  uint8_t code1 = frame & 0xff;

  // If either half is a kcode no triggers
  if(code0 == LCB::K0 || code0 == LCB::K1 ||
     code0 == LCB::K2 || code0 == LCB::K3) {
    return 0;
  }

  if(code1 == LCB::K0 || code1 == LCB::K1 ||
     code1 == LCB::K2 || code1 == LCB::K3) {
    return 0;
  }

  // Find 12-bit decoded version
  uint16_t value = (SixEight::decode(code0) << 6) | SixEight::decode(code1);
  if(((value>>7) & 0x1f) == 0) {
    // No BCR, or triggers, so part of a command
    return 0;
  }

  // How many triggers in mask (may be 0 if BCR)
  unsigned int count = 0;
  for(unsigned int i=0; i<4; i++) {
    count += (value>>(7+i)) & 0x1;
  }
  return count;
}

void StarChipsetEmu::countHits(AbcCfg& abc, const StripData& hits) const {
  if (not m_startHitCount) return;

  bool EnCount = abc.getSubRegisterValue("ENCOUNT");
  if (not EnCount) return;

  // HitCountReg0-63: four channels per register
  for (int ireg = 0; ireg < 64; ++ireg) {
    // Read HitCount Register
    // address
    unsigned addr = ireg + (+ABCStarRegister::HitCountREG0)._to_integral();
    auto reg = ABCStarRegister(ABCStarRegs::_from_integral(addr));

    // value
    unsigned counts = abc.getRegisterValue(reg);

    // Compute increments that should be added to the current counts
    unsigned incr = 0;

    // Four front-end channels: [ireg*4], [ireg*4+1], [ireg*4+2], [ireg*4+3]
    for (int ich = 0; ich < 4; ++ich) {
      // check if the counts for this channel has already reached maximum
      if ( ((counts>>(8*ich)) & 0xff) == 0xff ) continue;

      bool ahit = hits[ireg*4+ich];
      if (ahit)
        incr += (1 << (8*ich));
    }

    // Update HitCountReg
    abc.setRegisterValue(reg, counts+incr);
  }
}

void StarChipsetEmu::ackPulseCmd(int pulseType, uint8_t cmdBC) {
  // 8-bit BCID
  auto bcid = L0BufData( (m_bccnt + cmdBC)%256 );
  // L0 buffer address
  uint16_t addr = (m_bccnt + cmdBC) % L0BufDepth;

  m_l0buffer_lite[addr] = L0BufData(pulseType)<<8 | bcid;
  m_ndata_l0buf += 1;
}

void StarChipsetEmu::clearFEData() {
  for (size_t i=0; i<L0BufDepth; i++)
    m_l0buffer_lite[i].reset();

  for (auto& evtbuffer : m_evtbuffers_lite) {
    evtbuffer.second.fill(EvtBufData(0));
  }
}

void StarChipsetEmu::fillL0Buffer() {
  // Fill m_l0buffer_lite with no pulse bits and bcid for each LCB frame (4 BCs)
  for (unsigned ibc = 0; ibc < 4; ++ibc) {
    // 8-bit BCID
    auto bcid = L0BufData( (m_bccnt + ibc)%256 );

    // Address of L0 buffer to write
    unsigned addr = (m_bccnt + ibc) % L0BufDepth;

    m_l0buffer_lite[addr] = bcid;
  }
}

StarChipsetEmu::StripData StarChipsetEmu::getMasks(const AbcCfg& abc) {
  // mask registers
  unsigned maskinput0 = abc.getRegisterValue(ABCStarRegister::MaskInput0);
  unsigned maskinput1 = abc.getRegisterValue(ABCStarRegister::MaskInput1);
  unsigned maskinput2 = abc.getRegisterValue(ABCStarRegister::MaskInput2);
  unsigned maskinput3 = abc.getRegisterValue(ABCStarRegister::MaskInput3);
  unsigned maskinput4 = abc.getRegisterValue(ABCStarRegister::MaskInput4);
  unsigned maskinput5 = abc.getRegisterValue(ABCStarRegister::MaskInput5);
  unsigned maskinput6 = abc.getRegisterValue(ABCStarRegister::MaskInput6);
  unsigned maskinput7 = abc.getRegisterValue(ABCStarRegister::MaskInput7);

  // The following channel number convention is what is described in Table 9-4 of the ABCStar spec v7.8, but is NOT what is seen on the actual chip.
  /*
  StripData masks =
    (StripData(maskinput7) << 32*7) | (StripData(maskinput6) << 32*6) |
    (StripData(maskinput5) << 32*5) | (StripData(maskinput4) << 32*4) |
    (StripData(maskinput3) << 32*3) | (StripData(maskinput2) << 32*2) |
    (StripData(maskinput1) << 32*1) | (StripData(maskinput0));
  */
  /**/
  // To make the masks consistent with what is observed on actual chips
  StripData masks;
  for (size_t j = 0; j < 16; ++j) {
    // maskinput0
    // even bit
    if ( (maskinput0 >> 2*j) & 1 ) masks.set(j);
    // odd bit
    if ( (maskinput0 >> (2*j+1)) & 1 ) masks.set(128+j);
    // maskinput1
    // even bit
    if ( (maskinput1 >> 2*j) & 1 ) masks.set(j+16*1);
    // odd bit
    if ( (maskinput1 >> (2*j+1)) & 1 ) masks.set(128+j+16*1);
    // maskinput2
    // even bit
    if ( (maskinput2 >> 2*j) & 1 ) masks.set(j+16*2);
    // odd bit
    if ( (maskinput2 >> (2*j+1)) & 1 ) masks.set(128+j+16*2);
    // maskinput3
    // even bit
    if ( (maskinput3 >> 2*j) & 1 ) masks.set(j+16*3);
    // odd bit
    if ( (maskinput3 >> (2*j+1)) & 1 ) masks.set(128+j+16*3);
    // maskinput4
    // even bit
    if ( (maskinput4 >> 2*j) & 1 ) masks.set(j+16*4);
    // odd bit
    if ( (maskinput4 >> (2*j+1)) & 1 ) masks.set(128+j+16*4);
    // maskinput5
    // even bit
    if ( (maskinput5 >> 2*j) & 1 ) masks.set(j+16*5);
    // odd bit
    if ( (maskinput5 >> (2*j+1)) & 1 ) masks.set(128+j+16*5);
    // maskinput6
    // even bit
    if ( (maskinput6 >> 2*j) & 1 ) masks.set(j+16*6);
    // odd bit
    if ( (maskinput6 >> (2*j+1)) & 1 ) masks.set(128+j+16*6);
    // maskinput7
    // even bit
    if ( (maskinput7 >> 2*j) & 1 ) masks.set(j+16*7);
    // odd bit
    if ( (maskinput7 >> (2*j+1)) & 1 ) masks.set(128+j+16*7);
  }
  /**/

  return masks;
}

StarChipsetEmu::StripData StarChipsetEmu::getCalEnables(const AbcCfg& abc) {
  // Calibration enable registers
  unsigned calenable0 = abc.getRegisterValue(ABCStarRegister::CalREG0);
  unsigned calenable1 = abc.getRegisterValue(ABCStarRegister::CalREG1);
  unsigned calenable2 = abc.getRegisterValue(ABCStarRegister::CalREG2);
  unsigned calenable3 = abc.getRegisterValue(ABCStarRegister::CalREG3);
  unsigned calenable4 = abc.getRegisterValue(ABCStarRegister::CalREG4);
  unsigned calenable5 = abc.getRegisterValue(ABCStarRegister::CalREG5);
  unsigned calenable6 = abc.getRegisterValue(ABCStarRegister::CalREG6);
  unsigned calenable7 = abc.getRegisterValue(ABCStarRegister::CalREG7);

  // The following channel number convention is what is described in Table 9-4 of the ABCStar spec v7.8, but is NOT what is seen on the actual chip.
  /*  
  StripData enables =
    (StripData(calenable7) << 32*7) | (StripData(calenable6) << 32*6) |
    (StripData(calenable5) << 32*5) | (StripData(calenable4) << 32*4) |
    (StripData(calenable3) << 32*3) | (StripData(calenable2) << 32*2) |
    (StripData(calenable1) << 32*1) | (StripData(calenable0));
  */
  /**/
  // To make the masks consistent with what is observed on actual chips
  // Note: the following channel mapping for CalREGs is not the same as that for mask registers in StarEmu::getMasks either.
  StripData enables;
  for (size_t j = 0; j < 8; ++j) { // deal with 4 bits at a time
    // calenable0
    if ( (calenable0 >> 4*j) & 1 ) enables.set(2*j);
    if ( (calenable0 >> (4*j+1)) & 1 ) enables.set(2*j+1);
    if ( (calenable0 >> (4*j+2)) & 1 ) enables.set(128+2*j);
    if ( (calenable0 >> (4*j+3)) & 1 ) enables.set(128+2*j+1);
    // calenable1
    if ( (calenable1 >> 4*j) & 1 ) enables.set(2*j+16*1);
    if ( (calenable1 >> (4*j+1)) & 1 ) enables.set(2*j+1+16*1);
    if ( (calenable1 >> (4*j+2)) & 1 ) enables.set(128+2*j+16*1);
    if ( (calenable1 >> (4*j+3)) & 1 ) enables.set(128+2*j+1+16*1);
    // calenable2
    if ( (calenable2 >> 4*j) & 1 ) enables.set(2*j+16*2);
    if ( (calenable2 >> (4*j+1)) & 1 ) enables.set(2*j+1+16*2);
    if ( (calenable2 >> (4*j+2)) & 1 ) enables.set(128+2*j+16*2);
    if ( (calenable2 >> (4*j+3)) & 1 ) enables.set(128+2*j+1+16*2);
    // calenable3
    if ( (calenable3 >> 4*j) & 1 ) enables.set(2*j+16*3);
    if ( (calenable3 >> (4*j+1)) & 1 ) enables.set(2*j+1+16*3);
    if ( (calenable3 >> (4*j+2)) & 1 ) enables.set(128+2*j+16*3);
    if ( (calenable3 >> (4*j+3)) & 1 ) enables.set(128+2*j+1+16*3);
    // calenable4
    if ( (calenable4 >> 4*j) & 1 ) enables.set(2*j+16*4);
    if ( (calenable4 >> (4*j+1)) & 1 ) enables.set(2*j+1+16*4);
    if ( (calenable4 >> (4*j+2)) & 1 ) enables.set(128+2*j+16*4);
    if ( (calenable4 >> (4*j+3)) & 1 ) enables.set(128+2*j+1+16*4);
    // calenable5
    if ( (calenable5 >> 4*j) & 1 ) enables.set(2*j+16*5);
    if ( (calenable5 >> (4*j+1)) & 1 ) enables.set(2*j+1+16*5);
    if ( (calenable5 >> (4*j+2)) & 1 ) enables.set(128+2*j+16*5);
    if ( (calenable5 >> (4*j+3)) & 1 ) enables.set(128+2*j+1+16*5);
    // calenable6
    if ( (calenable6 >> 4*j) & 1 ) enables.set(2*j+16*6);
    if ( (calenable6 >> (4*j+1)) & 1 ) enables.set(2*j+1+16*6);
    if ( (calenable6 >> (4*j+2)) & 1 ) enables.set(128+2*j+16*6);
    if ( (calenable6 >> (4*j+3)) & 1 ) enables.set(128+2*j+1+16*6);
    // calenable7
    if ( (calenable7 >> 4*j) & 1 ) enables.set(2*j+16*7);
    if ( (calenable7 >> (4*j+1)) & 1 ) enables.set(2*j+1+16*7);
    if ( (calenable7 >> (4*j+2)) & 1 ) enables.set(128+2*j+16*7);
    if ( (calenable7 >> (4*j+3)) & 1 ) enables.set(128+2*j+1+16*7);
  }
  /**/
  return enables;
}

std::pair<uint8_t, StarChipsetEmu::StripData> StarChipsetEmu::generateFEData_StaticTest(const AbcCfg& abc, unsigned l0addr) {
  // Use mask bits as the hit pattern in Static Test mode
  StripData masks = getMasks(abc);

  uint8_t bcid = l0addr & 0xff;
  return std::make_pair(bcid, masks);
}

std::pair<uint8_t, StarChipsetEmu::StripData> StarChipsetEmu::generateFEData_TestPulse(const AbcCfg& abc, unsigned l0addr) {
  StripData hits;
  uint8_t bcid = 0;

  // enable
  bool TestPulseEnable = abc.getSubRegisterValue("TEST_PULSE_ENABLE");
  if (not TestPulseEnable)
    return std::make_pair(bcid, hits);

  // mask
  StripData masks = getMasks(abc);

  // Two test pulse options: determined by bit 18 of ABC register CREG0
  bool testPattEnable = abc.getSubRegisterValue("TESTPATT_ENABLE");
  if (testPattEnable) { // Use test pattern
    // Need to check four slots in m_l0buffer_lite: from l0addr to l0addr-3
    for (unsigned ibit=0; ibit<4; ibit++) {
      unsigned iaddr = (L0BufDepth + l0addr - ibit) % L0BufDepth;
      auto pulse = m_l0buffer_lite[iaddr];
      // top two bits: a digital test pulse if it is 2
      uint8_t pulsetype = (pulse.to_ulong()>>8) & 3;
      if (pulsetype == 2) { // there is a test pulse
        bcid = pulse.to_ulong() & 0xff;

        // testPatt1 if mask bit is 0, otherwise testPatt2
        uint8_t testPatt1 = abc.getSubRegisterValue("TESTPATT1");
        uint8_t testPatt2 = abc.getSubRegisterValue("TESTPATT2");

        StripData patt1_ibit(0); // Initialize all bits to zero
        if ( (testPatt1>>ibit)&1 ) {
          // set all bits to one if the i-th bit of testPatt1 is 1
          patt1_ibit.set();
        }

        StripData patt2_ibit(0); // Initialize all bits to zero
        if ( (testPatt2>>ibit)&1 ) {
          // set all bits to one if the i-th bit of testPatt2 is 1
          patt2_ibit.set();
        }

        // Use testpatt1 bit if a channel is unmasked
        // otherwise use testpatt2 bit
        hits = ~masks & patt1_ibit | masks & patt2_ibit;

        bcid += ibit;// assert(bcid == (l0addr&0xff));
        break;
      } // end of if there is a test pulse
    } // end of ibit loop

  } else { // One BC pulse based on mask bits
    auto pulse = m_l0buffer_lite[l0addr];
    uint8_t pulsetype = (pulse.to_ulong()>>8) & 3;
    if (pulsetype == 2) {
      bcid = pulse.to_ulong() & 0xff;// assert(bcid == (l0addr&0xff));
      hits = masks;
    }
  }

  return std::make_pair(bcid, hits);
}

std::pair<uint8_t, StarChipsetEmu::StripData> StarChipsetEmu::generateFEData_CaliPulse(const AbcCfg& abc, unsigned l0addr) {
  StripData hits;

  // read the L0 pipeline
  auto pulse = m_l0buffer_lite[l0addr];
  // lowest eight bits are for BCID
  uint8_t bcid = pulse.to_ulong() & 0xff;
  // top two bits indicate pulse type. Calibration pulse is 1.
  uint8_t pulsetype = (pulse.to_ulong()>>8) & 3;

  //assert(TM==0)
  bool CalPulseEnable = abc.getSubRegisterValue("CALPULSE_ENABLE");

  // Charge injection DAC
  uint16_t BCAL;
  if (pulsetype == 1 and CalPulseEnable) {
    BCAL = abc.getSubRegisterValue("BCAL");
    //assert(bcid == (l0addr&0xff));
  } else { // No calibration pulse. Hits could still be recorded due to noise.
    BCAL = 0;
    // assign BCID
    bcid = l0addr & 0xff;
  }

  // Threshold DAC
  // BVT: 8 bits, 0 - -550 mV
  uint8_t BVT = abc.getSubRegisterValue("BVT");

  // Trim Range
  // BTRANGE: 5 bits, 50 mV - 230 mV
  uint8_t BTRANGE = abc.getSubRegisterValue("BTRANGE");

  // Calibration enables for each strip channel
  auto enables = getCalEnables(abc);

  // Loop over 256 strips
  for (int istrip = 0; istrip < 256; ++istrip) {
    // TrimDAC
    uint8_t TrimDAC = abc.getTrimDACRaw(istrip);

    if (not enables[istrip])
      BCAL = 0;

    bool aHit = m_stripArray[istrip].calculateHit(BCAL, BVT, TrimDAC, BTRANGE);
    hits.set(istrip, aHit);
  }

  // apply masks
  auto masks = getMasks(abc);
  hits &= masks.flip();

  return std::make_pair(bcid, hits);
}

unsigned StarChipsetEmu::getL0BufferAddr(const AbcCfg& abc, uint8_t cmdBC) const {
  // L0A latency from ABCStar register CREG2
  // 9 bits
  unsigned l0_latency = abc.getSubRegisterValue("LATENCY");

  // cmdBC = 0, 1, 2, or 3 from trigger command
  // address of m_l0buffer_lite that associates to cmdBC
  // L0BufDepth in the bracket ensures the sum is positive
  return (L0BufDepth + m_bccnt - 4 + cmdBC - l0_latency) % L0BufDepth;
}

std::pair<uint8_t, StarChipsetEmu::StripData> StarChipsetEmu::getFEData(const AbcCfg& abc, unsigned l0addr) {
  // Mode of operation
  uint8_t TM = abc.getSubRegisterValue("TM");
  if (TM == 0) { // Normal data taking
    return generateFEData_CaliPulse(abc, l0addr);
  } else if (TM == 1) { // Static test mode
    return generateFEData_StaticTest(abc, l0addr);
  } else { // TM == 2: Test pulse mode
    return generateFEData_TestPulse(abc, l0addr);
  }
}

std::vector<uint16_t> StarChipsetEmu::getClusters(const AbcCfg& abc, const StripData& hits) {
  // max clusters
  bool maxcluster_en = abc.getSubRegisterValue("MAX_CLUSTER_ENABLE");
  uint8_t maxcluster = maxcluster_en ? abc.getSubRegisterValue("MAX_CLUSTER") : 63;
  return clusterFinder(hits, maxcluster);
}

std::vector<uint16_t> StarChipsetEmu::clusterFinder(
  const StripData& inputData, const uint8_t maxCluster) {
  std::vector<uint16_t> clusters;

  // The 256 strips are divided into two rows to form clusters
  // Split input data into uint64_t
  StripData selector(0xffffffffffffffffULL); // 64 ones
  // Row 1
  uint64_t d0l = (inputData & selector).to_ullong(); // 0 ~ 63
  uint64_t d0h = ((inputData >> 64) & selector).to_ullong(); // 64 ~ 127
  // Row 2
  uint64_t d1l = ((inputData >> 128) & selector).to_ullong(); // 128 ~ 191
  uint64_t d1h = ((inputData >> 192) & selector).to_ullong(); // 192 ~ 155

  while (d0l or d0h or d1l or d1h) {
    if (clusters.size() > maxCluster) break;

    uint16_t cluster1 = clusterFinder_sub(d1h, d1l, true);
    if (cluster1 != 0x3ff) // if not an empty cluster
      clusters.push_back(cluster1);

    if (clusters.size() > maxCluster)  break;

    uint16_t cluster0 = clusterFinder_sub(d0h, d0l, false);
    if (cluster0 != 0x3ff) // if not an empty cluster
      clusters.push_back(cluster0);
  }

  if (clusters.empty()) {
    clusters.push_back(0x3fe); // "no cluster byte"
  } else {
    // set last cluster bit
    clusters.back() |= 1 << 11;
  }

  return clusters;
}

uint16_t StarChipsetEmu::clusterFinder_sub(
  uint64_t& hits_high64, uint64_t& hits_low64, bool isSecondRow) {
  uint8_t hit_addr = 128;
  uint8_t hitpat_next3 = 0;

  // Count trailing zeros to get address of the hit
  if (hits_low64) {   
    hit_addr = __builtin_ctzll(hits_low64);
  } else if (hits_high64) {
    hit_addr = __builtin_ctzll(hits_high64) + 64;
  }

  // Get the value of the next three strips: [hit_addr+3: hit_addr+1]
  hitpat_next3 = getBit_128b(hit_addr+1, hits_high64, hits_low64) << 2
    | getBit_128b(hit_addr+2, hits_high64, hits_low64) << 1
    | getBit_128b(hit_addr+3, hits_high64, hits_low64);

  // Mask the bits that have already been considered
  // i.e. set bits [hit_addr+3 : hit_addr] to zero
  for (int i=0; i<4; ++i)
    setBit_128b(hit_addr+i, 0, hits_high64, hits_low64);

  if (hit_addr == 128) { // no cluster found
    return 0x3ff;
  } else {
    hit_addr += isSecondRow<<7;
    // set the lowest bit of any valid cluster to 0
    return hit_addr << 3 | hitpat_next3;
  }
}

inline bool StarChipsetEmu::getBit_128b(uint8_t bit_addr, uint64_t data_high64,
                                        uint64_t data_low64) {
  if (bit_addr > 127) return false;

  return bit_addr<64 ? data_low64>>bit_addr & 1 : data_high64>>(bit_addr-64) & 1;
}

inline void StarChipsetEmu::setBit_128b(uint8_t bit_addr, bool value,
                                        uint64_t& data_high64,
                                        uint64_t& data_low64) {
  if (bit_addr < 64) {
    data_low64 = (data_low64 & ~(1ULL << bit_addr)) | ((uint64_t)value << bit_addr);
  } else if (bit_addr < 128) {
    data_high64 =
      (data_high64 & ~(1ULL << (bit_addr-64))) | ((uint64_t)value << (bit_addr-64));
  }
}
