#include "StarEmu.h"

#include <chrono>
#include <iomanip>
#include <fstream>
#include <thread>

#include "AllHwControllers.h"
#include "EmuCom.h"
#include "EmuRxCore.h"
#include "LCBUtils.h"
#include "RingBuffer.h"

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

auto logger = logging::make_log("StarEmu");
}

StarEmu::StarEmu(ClipBoard<RawData> &rx, EmuCom * tx, std::string json_file_path,
    unsigned hpr_period)
    : m_txRingBuffer ( tx )
    , m_rxQueue ( rx )
    , m_bccnt( 0 )
    , m_starCfg( new StarCfg )
    , HPRPERIOD( hpr_period )
{
    run = true;

    if (!json_file_path.empty()) {
      std::ifstream file(json_file_path);
      json j = json::parse(file);
      file.close();

      // Initialize FE strip array from config json
      for (size_t istrip = 0; istrip < 256; ++istrip) {
          m_stripArray[istrip].setValue(j["vthreshold_mean"][istrip],
                                        j["vthreshold_sigma"][istrip],
                                        j["noise_occupancy_mean"][istrip],
                                        j["noise_occupancy_sigma"][istrip]);
      }
    }

    m_ignoreCmd = true;
    m_isForABC = false;

    m_startHitCount = false;
    m_bc_sel = 0;
    
    // HCCStar and ABCStar configurations
    // TODO: should get these from chip config json file
    // m_starCfg->fromFileJson(j_starCfg);
    // for now
    m_starCfg->setHCCChipId(15);
    m_starCfg->addABCchipID(15);
    /*
    for (size_t id_abc = 1; id_abc < 9; id_abc++) {
        m_starCfg->addABCchipID(id_abc);
    }
    */

    hpr_clkcnt = HPRPERIOD/2; // 20000 BCs or 500 us
    hpr_sent.resize(m_starCfg->numABCs() + 1); // 1 HCCStar + nABCs ABCStar chips
    std::fill(hpr_sent.begin(), hpr_sent.end(), false);

    m_l0buffers_lite.clear();
    m_l0buffers_lite.resize(m_starCfg->numABCs());

    // Preload test pattern
    for (auto& l0buffer : m_l0buffers_lite) {
        for (StripData& fe_hits : l0buffer) {
            fe_hits = (StripData(0xfffe0000) << (32*7)) |
                (StripData(0xfffe0000) << (32*3));
        }
    }
    // expected clusterFinder output per ABCStar:
    // {0x78f, 0x38f, 0x7af, 0x3af, 0x7cf, 0x3cf, 0x7ee, 0x3ee}
    ////////////////////////////////////////////////////////////////////////
}

StarEmu::~StarEmu() {}

void StarEmu::sendPacket(uint8_t *byte_s, uint8_t *byte_e) {
    int byte_length = byte_e - byte_s;

    int word_length = (byte_length + 3) / 4;

    uint32_t *buf = new uint32_t[word_length];

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

    std::unique_ptr<RawData> data(new RawData(0, buf, word_length));

    m_rxQueue.pushData(std::move(data));
}

//
// Build data packets
//
bool StarEmu::getParity_8bits(uint8_t val)
{
    val ^= val >> 4;
    val ^= val >> 2;
    val ^= val >> 1;
    return val&1;
}

std::vector<uint8_t> StarEmu::buildPhysicsPacket(
    const std::vector<std::vector<uint16_t>>& allClusters,
    PacketTypes typ, uint8_t l0tag, uint8_t bc_count, uint16_t endOfPacket)
{
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
    for (int ichannel=0; ichannel<allClusters.size(); ++ichannel) {
        for ( uint16_t cluster : allClusters[ichannel]) {
            // cluster bits:
            // "0" + 4-bit channel number + 11-bit cluster dropping the last cluster bit
            uint16_t clusterbits = (ichannel & 0xf)<<11 | (cluster & 0x7ff);
            data_packets.push_back((clusterbits>>8) & 0xff);
            data_packets.push_back(clusterbits & 0xff);
        }
    }

    // Todo: error block

    // Fixed 16-bit end of packet cluster pattern
    data_packets.push_back((endOfPacket>>8) & 0xff);
    data_packets.push_back(endOfPacket & 0xff);

    return data_packets;
}

std::vector<uint8_t> StarEmu::buildABCRegisterPacket(
    PacketTypes typ, uint8_t input_channel, uint8_t reg_addr, unsigned reg_data,
    uint16_t reg_status)
{
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

std::vector<uint8_t> StarEmu::buildHCCRegisterPacket(PacketTypes typ, uint8_t reg_addr, unsigned reg_data)
{
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
// Decode LCB
//
void StarEmu::DecodeLCB(LCB::Frame frame) {

    SPDLOG_LOGGER_TRACE(logger, "Raw LCB frame = 0x{:x} BC = {}", frame, m_bccnt);

    // HPR
    doHPR(frame);

    // {code0, code1}
    uint8_t code0 = (frame >> 8) & 0xff;
    uint8_t code1 = frame & 0xff;

    bool iskcode0 = SixEight::is_kcode(code0);
    bool iskcode1 = SixEight::is_kcode(code1);

    if (not (iskcode0 or iskcode1) ) {
        // Neither of the 8-bit symbol is a kcode
        // Decode the 16-bit frame to the 12-bit data
        uint16_t data12 = (SixEight::decode(code0) << 6) | SixEight::decode(code1);
        
        if ( (data12 >> 7) & 0x1f ) {
            // Top 5 bits are not zeros: has a BCR and/or triggers
            doL0A(data12);
        }
        else {
            // Top 5 bits are all zeros: part of a command sequence
            doRegReadWrite(frame);
        }
    }
    else {
        // Kcode detected
        if (code0 == LCB::K3) { // Fast command
            // decode the second symbol
            uint8_t k3cmd = SixEight::decode(code1);
            doFastCommand(k3cmd);
        }
        else if (code0 == LCB::K2) { // Start or end of a command sequence

            doRegReadWrite(frame);
        }
        else if (frame == LCB::IDLE) { // Idle
            SPDLOG_LOGGER_TRACE(logger, "Receive an IDLE");
            // do nothing
        }
    } // if (not (iskcode0 or iskcode1) )

    // Increment BC counter
    m_bccnt += 4;
}

//
// Register commands
//
void StarEmu::doRegReadWrite(LCB::Frame frame) {
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

void StarEmu::writeRegister(const uint32_t data, const uint8_t address,
                            bool isABC, const unsigned ABCID)
{
    if (isABC) {
        m_starCfg->setABCRegister(address, data, ABCID);
    }
    else {
        m_starCfg->setHCCRegister(address, data);
    }
}

void StarEmu::readRegister(const uint8_t address, bool isABC,
                           const unsigned ABCID)
{
    if (isABC) { // Read ABCStar registers
        PacketTypes ptype = PacketTypes::ABCRegRd;

        // HCCStar channel number
        unsigned ich = m_starCfg->hccChannelForABCchipID(ABCID);
        if (ich >= m_starCfg->numABCs()) {
            logger->warn("Cannot find an ABCStar chip with ID = {}", ABCID);
            return;
        }

        // read register
        unsigned data = m_starCfg->getABCRegister(address, ABCID);

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
    }
    else { // Read HCCStar registers
        PacketTypes ptype = PacketTypes::HCCRegRd;

        // read register
        unsigned data = m_starCfg->getHCCRegister(address);

        // build and send data packet
        auto packet = buildHCCRegisterPacket(ptype, address, data);
        sendPacket(packet);
    }
}

void StarEmu::execute_command_sequence()
{
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
            for (int index=1; index <= m_starCfg->numABCs(); ++index)
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
void StarEmu::doFastCommand(uint8_t data6) {
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
        for (int ichip=1; ichip <= m_starCfg->numABCs(); ++ichip) {
            this->generateFEData_CaliPulse(ichip, bcsel);
        }
        break;
    case LCB::ABC_DIGITAL_PULSE :
        for (int ichip=1; ichip <= m_starCfg->numABCs(); ++ichip) {
            this->generateFEData_TestPulse(ichip, bcsel);
        }
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

void StarEmu::logicReset()
{
    m_bccnt = 0;
    hpr_clkcnt = HPRPERIOD/2;
    std::fill(hpr_sent.begin(), hpr_sent.end(), false);
    m_starCfg->setSubRegisterValue(0, "TESTHPR", 0);
    m_starCfg->setSubRegisterValue(0, "STOPHPR", 0);
    m_starCfg->setSubRegisterValue(0, "MASKHPR", 0);
    
    for (unsigned ichip = 1; ichip <= m_starCfg->numABCs(); ++ichip) {
        clearFEData(ichip);

        m_starCfg->setSubRegisterValue(ichip, "TESTHPR", 0);
        m_starCfg->setSubRegisterValue(ichip, "STOPHPR", 0);
        m_starCfg->setSubRegisterValue(ichip, "MASKHPR", 0);
    }
}

/////////////////////////////////////////////
// Move the reset functions below to StarCfg?
void StarEmu::resetABCRegisters()
{
    for (unsigned ichip = 1; ichip <= m_starCfg->numABCs(); ++ichip) {
        unsigned abc = m_starCfg->getABCchipID(ichip);
        // cf. ABCStar specs v7.8 section 9.14
        m_starCfg->setABCRegister(ABCStarRegister::SCReg, 0x00000000, abc);

        m_starCfg->setABCRegister(ABCStarRegister::ADCS1, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::ADCS2, 0x000000ff, abc);
        m_starCfg->setABCRegister(ABCStarRegister::ADCS3, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::ADCS4, 0x0000010c, abc);
        //m_starCfg->setABCRegister(ABCStarRegister::ADCS5, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::ADCS6, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::ADCS7, 0x00000000, abc);

        m_starCfg->setABCRegister(ABCStarRegister::MaskInput0, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::MaskInput1, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::MaskInput2, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::MaskInput3, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::MaskInput4, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::MaskInput5, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::MaskInput6, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::MaskInput7, 0x00000000, abc);

        m_starCfg->setABCRegister(ABCStarRegister::CREG0, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CREG1, 0x00000004, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CREG2, 0x00000190, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CREG3, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CREG4, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CREG6, 0x0000ffff, abc);

        m_starCfg->setABCRegister(ABCStarRegister::STAT0, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::STAT1, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::STAT2, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::STAT3, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::STAT4, 0x00000000, abc);

        //m_starCfg->setABCRegister(ABCStarRegister::HPR, 0x00000000, abc);

        m_starCfg->setABCRegister(ABCStarRegister::CalREG0, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CalREG1, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CalREG2, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CalREG3, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CalREG4, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CalREG5, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CalREG6, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::CalREG7, 0x00000000, abc);
    }

    resetABCTrimDAC();
    resetABCHitCounts();
}

void StarEmu::resetABCSEU()
{
    for (unsigned ichip = 1; ichip <= m_starCfg->numABCs(); ++ichip) {
        unsigned abc = m_starCfg->getABCchipID(ichip);
        m_starCfg->setABCRegister(ABCStarRegister::STAT0, 0x00000000, abc);
        m_starCfg->setABCRegister(ABCStarRegister::STAT1, 0x00000000, abc);
    }
}

void StarEmu::resetABCHitCounts()
{
    for (unsigned ichip = 1; ichip <= m_starCfg->numABCs(); ++ichip) {
        unsigned abc = m_starCfg->getABCchipID(ichip);
        // 64 HitCount registers per ABCStar
        for (size_t j = 0; j < 64; j++) {
            unsigned addr = (+ABCStarRegister::HitCountREG0)._to_integral() + j;
            m_starCfg->setABCRegister(addr, 0x00000000, abc);
        }
    }
}

void StarEmu::resetABCTrimDAC()
{
    for (unsigned ichip = 1; ichip <= m_starCfg->numABCs(); ++ichip) {
        unsigned abc = m_starCfg->getABCchipID(ichip);
        // 40 TrimDAC registers per ABCStar
        for (size_t j = 0; j < 40; j++) {
            unsigned addr = (+ABCStarRegister::TrimDAC0)._to_integral() + j;
            m_starCfg->setABCRegister(addr, 0x00000000, abc);
        }
    }
}

void StarEmu::resetSlowCommand()
{
    m_ignoreCmd = true;
    m_isForABC = false;

    // clear command buffer
    std::queue<uint8_t> empty_buffer;
    std::swap(m_reg_cmd_buffer, empty_buffer);
}

void StarEmu::resetHCCRegisters()
{
    // cf. HCCStar specs v1.0e section 9.15.1
    m_starCfg->setHCCRegister(HCCStarRegister::Pulse,     0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::Delay1,    0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::Delay2,    0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::Delay3,    0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::DRV1,      0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::DRV2,      0x00000014);
    m_starCfg->setHCCRegister(HCCStarRegister::ICenable,  0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::OPmode,    0x00020001);
    m_starCfg->setHCCRegister(HCCStarRegister::OPmodeC,   0x00020001);
    m_starCfg->setHCCRegister(HCCStarRegister::Cfg1,      0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::Cfg2,      0x0000018e);
    m_starCfg->setHCCRegister(HCCStarRegister::ExtRst,    0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::ExtRstC,   0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::ErrCfg,    0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::ADCcfg,    0x00406600);
}

void StarEmu::resetHCCSEU()
{
    m_starCfg->setHCCRegister(HCCStarRegister::SEU1, 0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::SEU2, 0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::SEU3, 0x00000000);
}

void StarEmu::resetHCCPLL()
{
    m_starCfg->setHCCRegister(HCCStarRegister::PLL1, 0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::PLL2, 0x00000000);
    m_starCfg->setHCCRegister(HCCStarRegister::PLL3, 0x00000000);
}
/////////////////////////////////////////////

//
// HPR
//
void StarEmu::doHPR(LCB::Frame frame)
{
    doHPR_HCC(frame);

    for (unsigned ichip = 1; ichip <= m_starCfg->numABCs(); ++ichip) {
        doHPR_ABC(frame, ichip);
    }

    // Each LCB command frame covers 4 BCs
    hpr_clkcnt += 4;
}

void StarEmu::doHPR_HCC(LCB::Frame frame)
{
    //// Update the HPR register
    setHCCStarHPR(frame);

    //// HPR control logic
    bool testHPR = m_starCfg->getSubRegisterValue(0, "TESTHPR");
    bool stopHPR = m_starCfg->getSubRegisterValue(0, "STOPHPR");
    bool maskHPR = m_starCfg->getSubRegisterValue(0, "MASKHPR");

    // Assume for now in the software emulation LCB is always locked and only
    // testHPR bit can trigger the one-time pulse to send an HPR packet
    // The one-time pulse is ignored if maskHPR is one.
    bool lcb_lock_changed = testHPR & (~maskHPR);

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
            PacketTypes::HCCHPR,
            (+HCCStarRegister::HPR)._to_integral(),
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

void StarEmu::doHPR_ABC(LCB::Frame frame, unsigned ichip)
{
    int abcID = m_starCfg->getABCchipID(ichip);

    //// Update the HPR register
    setABCStarHPR(frame, abcID);

    //// HPR control logic
    bool testHPR = m_starCfg->getSubRegisterValue(ichip, "TESTHPR");
    bool stopHPR = m_starCfg->getSubRegisterValue(ichip, "STOPHPR");
    bool maskHPR = m_starCfg->getSubRegisterValue(ichip, "MASKHPR");

    bool lcb_lock_changed = testHPR & (~maskHPR);
    bool hpr_periodic = not (hpr_clkcnt%HPRPERIOD) and not stopHPR;
    bool hpr_initial = not hpr_sent[ichip] and stopHPR;

    //// Build and send HPR packets
    if (lcb_lock_changed or hpr_periodic or hpr_initial) {
        auto packet_abchpr = buildABCRegisterPacket(
            PacketTypes::ABCHPR, ichip-1, (+ABCStarRegister::HPR)._to_integral(),
            m_starCfg->getABCRegister(ABCStarRegister::HPR, abcID),
            (abcID&0xf) << 12);

        sendPacket(packet_abchpr);

        hpr_sent[ichip] = true;
    }

    //// Update HPR control bits
    if (stopHPR and lcb_lock_changed)
        m_starCfg->setSubRegisterValue(ichip, "STOPHPR", 0);
    if (testHPR)
        m_starCfg->setSubRegisterValue(ichip, "TESTHPR", 0);
}

void StarEmu::setHCCStarHPR(LCB::Frame frame)
{
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

void StarEmu::setABCStarHPR(LCB::Frame frame, int abcID)
{
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
void StarEmu::doL0A(uint16_t data12) {
    bool bcr = (data12 >> 11) & 1;  // BC reset
    uint8_t l0a_mask = (data12 >> 7) & 0xf; // 4-bit L0A mask
    uint8_t l0a_tag = data12 & 0x7f; // 7-bit L0A tag

    logger->debug("Receive an L0A command: BCR = {}, L0A mask = {:b}, L0A tag = 0x{:x}", bcr, l0a_mask, l0a_tag);

    // Prepare front-end data
    // Loop over all ABCStar chips
    for (int ichip = 1; ichip <= m_starCfg->numABCs(); ++ichip) 
        prepareFEData(ichip);
    
    // A LCB frame covers 4 BCs
    for (unsigned ibc = 0; ibc < 4; ++ibc) {
        // check if there is a L0A
        // msb of L0A mask corresponds to the earliest BC
        if ( (l0a_mask >> (3-ibc)) & 1 ) {
            // clusters to be sent
            std::vector<std::vector<uint16_t>> clusters;
            
            // Read all ABCStar chips
            for (unsigned iabc = 0; iabc < m_starCfg->numABCs(); ++iabc) {
                // count hits
                countHits(iabc, ibc);

                // find and add clusters
                addClusters(clusters, iabc, ibc);
            }

            // Get BCID associated with the event
            uint8_t bcid = getEventBCID(ibc) & 0xff;

            // build and send data packet
            PacketTypes ptype = PacketTypes::LP; // for now
            std::vector<uint8_t> packet =
                buildPhysicsPacket(clusters, ptype, l0a_tag+ibc, bcid);
            sendPacket(packet);
        }
    }
    
    if (bcr) m_bccnt = 0;
}

unsigned int StarEmu::countTriggers(LCB::Frame frame) {
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

void StarEmu::countHits(unsigned iABC, uint8_t cmdBC)
{
    if (not m_startHitCount) return;

    unsigned ichip = iABC+1;
    int abcID = m_starCfg->getABCchipID(ichip);
    bool EnCount = m_starCfg->getSubRegisterValue(ichip, "ENCOUNT");
    if (not EnCount) return;

    auto feBC = getL0BufferAddr(iABC, cmdBC);

    // HitCountReg0-63: four channels per register
    for (int ireg = 0; ireg < 64; ++ireg) {
        // Read HitCount Register
        // address
        unsigned addr = ireg + (+ABCStarRegister::HitCountREG0)._to_integral();
        // value
        unsigned counts = m_starCfg->getABCRegister(addr, abcID);
        
        // Compute increments that should be added to the current counts
        unsigned incr = 0;

        // Four front-end channels: [ireg*4], [ireg*4+1], [ireg*4+2], [ireg*4+3]
        for (int ich = 0; ich < 4; ++ich) {
            // check if the counts for this channel has already reached maximum
            if ( ((counts>>(8*ich)) & 0xff) == 0xff ) continue;

            bool ahit = m_l0buffers_lite[iABC][feBC][ireg*4+ich];
            if (ahit)
                incr += (1 << (8*ich));
        }

        // Update HitCountReg
        m_starCfg->setABCRegister(addr, counts+incr, abcID);
    }
}

uint8_t StarEmu::getEventBCID(uint8_t cmdBC)
{
    if (m_l0buffers_lite.size() == 0) return 0;

    // Get BCID from the first ABCStar. BCID should be the same in all ABCStars.
    auto l0addr = getL0BufferAddr(0, cmdBC);
    // Top 8 bits of StripData are event BCID
    auto bcid = (m_l0buffers_lite[0][l0addr] >> NStrips).to_ulong();

    return bcid & 0xff;
}

void StarEmu::clearFEData(unsigned ichip)
{
    // ABC index starts from 1. Zero is reserved for HCC.
    unsigned iABC = ichip - 1;
    
    for (int ibc = 0; ibc < 4; ++ibc)
        m_l0buffers_lite[iABC][ibc].reset();
}

StarEmu::StripData StarEmu::getMasks(unsigned ichip)
{
    int abcID = m_starCfg->getABCchipID(ichip);
    
    // mask registers
    unsigned maskinput0 = m_starCfg->getABCRegister(ABCStarRegister::MaskInput0, abcID);
    unsigned maskinput1 = m_starCfg->getABCRegister(ABCStarRegister::MaskInput1, abcID);
    unsigned maskinput2 = m_starCfg->getABCRegister(ABCStarRegister::MaskInput2, abcID);
    unsigned maskinput3 = m_starCfg->getABCRegister(ABCStarRegister::MaskInput3, abcID);
    unsigned maskinput4 = m_starCfg->getABCRegister(ABCStarRegister::MaskInput4, abcID);
    unsigned maskinput5 = m_starCfg->getABCRegister(ABCStarRegister::MaskInput5, abcID);
    unsigned maskinput6 = m_starCfg->getABCRegister(ABCStarRegister::MaskInput6, abcID);
    unsigned maskinput7 = m_starCfg->getABCRegister(ABCStarRegister::MaskInput7, abcID);

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

void StarEmu::applyMasks(unsigned ichip) {

    unsigned iABC = ichip - 1;

    uint8_t TM = m_starCfg->getSubRegisterValue(ichip, "TM");
    if (TM != 0) // do nothing if not normal data taking mode
        return;

    StripData masks = getMasks(ichip);

    for (int ibc = 0; ibc < 4; ++ibc)
        m_l0buffers_lite[iABC][ibc] &= masks.flip();
}

void StarEmu::generateFEData_StaticTest(unsigned ichip)
{
    // ABC index starts from 1. Zero is reserved for HCC.
    unsigned iABC = ichip - 1;

    uint8_t TM = m_starCfg->getSubRegisterValue(ichip, "TM");
    if (TM != 1) // do nothing if not static test mode
        return;

    // Use mask bits as the hit pattern in Static Test
    StripData masks = getMasks(ichip);
    
    for (int ibc = 0; ibc < 4; ++ibc) {
        // Add BCID
        StripData bcid = StripData(m_bccnt+ibc) << NStrips;

        // Mod 4: m_l0buffers_lite is of size 4 and is used as a ring buffer
        m_l0buffers_lite[iABC][ibc%4] = bcid | masks;
    }
}

void StarEmu::generateFEData_TestPulse(unsigned ichip, uint8_t cmdBC)
{ // Triggered by the "Digital Test Pulse" fast command

    // ABC index starts from 1. Zero is reserved for HCC.
    unsigned iABC = ichip - 1;

    uint8_t TM = m_starCfg->getSubRegisterValue(ichip, "TM");
    if (TM != 2) // do nothing if not test pulse mode
        return;
    
    // enable
    bool TestPulseEnable = m_starCfg->getSubRegisterValue(ichip, "TEST_PULSE_ENABLE");
    if (not TestPulseEnable)
        return;

    StripData masks = getMasks(ichip);

    // Two test pulse options: determined by bit 18 of ABC register CREG0
    bool testPattEnable = m_starCfg->getSubRegisterValue(ichip, "TESTPATT_ENABLE");
    
    if (testPattEnable) { // Use test pattern for four consecutive BC
        // Test patterns
        // testPatt1 if mask bit is 0, otherwise testPatt2
        uint8_t testPatt1 = m_starCfg->getSubRegisterValue(ichip, "TESTPATT1");
        uint8_t testPatt2 = m_starCfg->getSubRegisterValue(ichip, "TESTPATT2");

        for (int ibit = 0; ibit < 4; ++ibit) {
            StripData patt1_ibit(0); // 0's
            if ( (testPatt1 >> ibit) & 1 ) {
                patt1_ibit.set(); // 1's
                // Make top bits for BCID zeros 
                (patt1_ibit <<= NBitsBC) >>= NBitsBC;
            }

            StripData patt2_ibit(0); // 0's
            if ( (testPatt2 >> ibit) & 1 ) {
                patt2_ibit.set(); // 1's
                // Make top BCID bits zeros
                // (not needed here if top bits of masks are zeros)
                (patt2_ibit <<= NBitsBC) >>= NBitsBC;
            }

            // The pulse is generated on the cmdBC'th BC and the three next BCs
            unsigned ibc = cmdBC + ibit;

            // The top bits for BCID 
            StripData bcid = StripData(m_bccnt+ibc) << NStrips;

            // ibc%4: m_l0buffers_lite is of size 4 and is used as a ring buffer
            m_l0buffers_lite[iABC][ibc%4] = bcid |
                (~masks & patt1_ibit | masks & patt2_ibit);
        }
    }
    else { // One clock pulse using mask bits
        for (int ibc = 0; ibc < 4; ++ibc) {
            // BCID
            StripData bcid = StripData(m_bccnt+ibc) << NStrips;
            
            if (ibc == cmdBC) {
                m_l0buffers_lite[iABC][ibc] = bcid | masks;
            }
            else {
                m_l0buffers_lite[iABC][ibc] = bcid;
            }
        }
    } // if (testPattEnable)
}

void StarEmu::generateFEData_CaliPulse(unsigned ichip, uint8_t bc)
{ // Triggered by the "Calibration Pulse" fast command

    // ABC index starts from 1. Zero is reserved for HCC.
    unsigned iABC = ichip - 1;

    uint8_t TM = m_starCfg->getSubRegisterValue(ichip, "TM");
    if (TM != 0) // do nothing if not normal data taking mode
        return;
    
    bool CalPulseEnable = m_starCfg->getSubRegisterValue(ichip, "CALPULSE_ENABLE");
    if (not CalPulseEnable)
        return;
    
    // Injected charge DAC
    // 9 bits, 0 - 170 mV
    uint16_t BCAL = m_starCfg->getSubRegisterValue(ichip, "BCAL");

    // Threshold DAC
    // BVT: 8 bits, 0 - -550 mV
    uint8_t BVT = m_starCfg->getSubRegisterValue(ichip, "BVT");
    // Trim Range
    // BTRANGE: 5 bits, 50 mV - 230 mV
    uint8_t BTRANGE = m_starCfg->getSubRegisterValue(ichip, "BTRANGE");

    // Loop over 256 strips
    for (int istrip = 0; istrip < 256; ++istrip) {
        // TrimDAC
        unsigned col = istrip%128 + 1;
        unsigned row = istrip/128 + 2*ichip - 1;
        uint8_t TrimDAC = m_starCfg->getTrimDAC(col, row);

        bool aHit = m_stripArray[istrip].calculateHit(BCAL, BVT, TrimDAC, BTRANGE);
        
        if (aHit) { // has a hit
            m_l0buffers_lite[iABC][bc].set(istrip);
        }
        else { // no hit
            m_l0buffers_lite[iABC][bc].reset(istrip);
        }
    }

    // Calibration enables
    StripData enables = getCalEnables(ichip);
    m_l0buffers_lite[iABC][bc] &= enables;

    // Add BCID
    m_l0buffers_lite[iABC][bc] |= (StripData(m_bccnt+bc) << NStrips);
}

StarEmu::StripData StarEmu::getCalEnables(unsigned ichip)
{
    int abcID = m_starCfg->getABCchipID(ichip);
    unsigned iABC = ichip - 1;

    // Calibration enable registers
    unsigned calenable0 = m_starCfg->getABCRegister(ABCStarRegister::CalREG0, abcID);
    unsigned calenable1 = m_starCfg->getABCRegister(ABCStarRegister::CalREG1, abcID);
    unsigned calenable2 = m_starCfg->getABCRegister(ABCStarRegister::CalREG2, abcID);
    unsigned calenable3 = m_starCfg->getABCRegister(ABCStarRegister::CalREG3, abcID);
    unsigned calenable4 = m_starCfg->getABCRegister(ABCStarRegister::CalREG4, abcID);
    unsigned calenable5 = m_starCfg->getABCRegister(ABCStarRegister::CalREG5, abcID);
    unsigned calenable6 = m_starCfg->getABCRegister(ABCStarRegister::CalREG6, abcID);
    unsigned calenable7 = m_starCfg->getABCRegister(ABCStarRegister::CalREG7, abcID);
    
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

void StarEmu::prepareFEData(unsigned ichip)
{
    // ABC chip index starts from 1. Zero is reserved for HCC.
    assert(ichip);

    // Check mode of operation
    uint8_t TM = m_starCfg->getSubRegisterValue(ichip, "TM");
    
    if (TM == 0) { // Normal data taking
        this->applyMasks(ichip);
    }
    else if (TM == 1) { // Static test mode
        this->generateFEData_StaticTest(ichip);
    }
    // TM == 2: Test pulse mode
    // Digital or analog test pulses are generated by fast commands
}

std::vector<uint16_t> StarEmu::clusterFinder(
    const StripData& inputData, const uint8_t maxCluster)
{
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
    }
    else {
        // set last cluster bit
        clusters.back() |= 1 << 11;
    }

    return clusters;
}

uint16_t StarEmu::clusterFinder_sub(uint64_t& hits_high64, uint64_t& hits_low64,
                                    bool isSecondRow)
{
    uint8_t hit_addr = 128;
    uint8_t hitpat_next3 = 0;

    // Count trailing zeros to get address of the hit
    if (hits_low64) {   
        hit_addr = __builtin_ctzll(hits_low64);
    }
    else if (hits_high64) {
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
    }
    else {
        hit_addr += isSecondRow<<7;
        // set the lowest bit of any valid cluster to 0
        return hit_addr << 3 | hitpat_next3;
    }
}

inline bool StarEmu::getBit_128b(uint8_t bit_addr, uint64_t data_high64,
                                 uint64_t data_low64)
{
    if (bit_addr > 127) return false;

    return bit_addr<64 ? data_low64>>bit_addr & 1 : data_high64>>(bit_addr-64) & 1;
}

inline void StarEmu::setBit_128b(uint8_t bit_addr, bool value,
                                 uint64_t& data_high64, uint64_t& data_low64)
{
    if (bit_addr < 64) {
        data_low64 = (data_low64 & ~(1ULL << bit_addr)) | ((uint64_t)value << bit_addr);
    }
    else if (bit_addr < 128) {
        data_high64 =
            (data_high64 & ~(1ULL << (bit_addr-64))) | ((uint64_t)value << (bit_addr-64));
    }
}

void StarEmu::addClusters(std::vector<std::vector<uint16_t>>& allclusters,
                          unsigned iABC, uint8_t cmdBC)
{
    // Need to correct the BC from the L0A command by the L0A latency in order to
    // access the corresponding entries from m_l0buffers_lite[iABC] ring buffer
    auto feBC = getL0BufferAddr(iABC, cmdBC);

    // max clusters
    bool maxcluster_en = m_starCfg->getSubRegisterValue(iABC+1, "MAX_CLUSTER_ENABLE");
    uint8_t maxcluster = maxcluster_en ? m_starCfg->getSubRegisterValue(iABC+1, "MAX_CLUSTER") : 63;
    
    // Get front-end data and find clusters
    std::vector<uint16_t> abc_clusters = clusterFinder(
        m_l0buffers_lite[iABC][feBC], maxcluster);
    
    allclusters.push_back(abc_clusters);
}

unsigned StarEmu::getL0BufferAddr(unsigned iABC, uint8_t cmdBC)
{
    // cmdBC = 0, 1, 2, or 3 from L0A command
    
    // get L0A latency from the ABCStar config register CREG2
    // 9 bits, in unit of BC (25 ns)
    unsigned l0a_latency = m_starCfg->getSubRegisterValue(iABC+1, "LATENCY");

    // return address of the FE data in m_l0buffers_lite[iABC] that corresponds to cmdBC
    // m_l0buffers_lite[iABC] is of size 4 and is used as a ring buffer
    // 512 ensures value in the bracket is positive 
    return (512 + cmdBC - l0a_latency) % 4;
}

void StarEmu::executeLoop() {
    logger->info("Starting emulator loop");

    static const auto SLEEP_TIME = std::chrono::milliseconds(1);

    while (run) {
        if ( m_txRingBuffer->isEmpty()) {
            std::this_thread::sleep_for( SLEEP_TIME );
            continue;
        }

        logger->debug("{}: -----------------------------------------------------------", __PRETTY_FUNCTION__);

        uint32_t d = m_txRingBuffer->read32();

        
        uint16_t d0 = (d >> 16) & 0xffff;
        uint16_t d1 = (d >> 0) & 0xffff;

        DecodeLCB(d0);
        DecodeLCB(d1);

        /*
        int trig_count = 0;
        trig_count += countTriggers(d0);
        trig_count += countTriggers(d1);

        // This is an LP packet
        alignas(32) uint8_t fixed_packet[] =
            {0x20, 0x06,
             // Channel 0...
             0x07, 0x8f, 0x03, 0x8f, 0x07, 0xaf, 0x03, 0xaf,
             // Channel 1...
             0x0f, 0x8f, 0x0b, 0x8f, 0x0f, 0xaf, 0x0b, 0xaf,
             // Channel 2
             0x17, 0x8f, 0x13, 0x8f, 0x17, 0xaf, 0x13, 0xaf,
             0x17, 0xcf, 0x13, 0xcf, 0x17, 0xee, 0x13, 0xee,
             // Channel 0 continued
             0x07, 0xcf, 0x03, 0xcf, 0x07, 0xee, 0x03, 0xee,
             // Channel 1 continued
             0x0f, 0xcf, 0x0b, 0xcf, 0x0f, 0xee, 0x0b, 0xee,
             // Channel 3
             0x1f, 0x8f, 0x1b, 0x8f, 0x1f, 0xaf, 0x1b, 0xaf,
             0x1f, 0xcf, 0x1b, 0xcf, 0x1f, 0xee, 0x1b, 0xee,
             // Channel 4
             0x27, 0x8f, 0x23, 0x8f, 0x27, 0xaf, 0x23, 0xaf,
             0x27, 0xcf, 0x23, 0xcf, 0x27, 0xee, 0x23, 0xee,
             // Channel 5
             0x2f, 0x8f, 0x2b, 0x8f, 0x2f, 0xaf, 0x2b, 0xaf,
             0x2f, 0xcf, 0x2b, 0xcf, 0x2f, 0xee, 0x2b, 0xee,
             // Channel 6
             0x37, 0x8f, 0x33, 0x8f, 0x37, 0xaf, 0x33, 0xaf,
             0x37, 0xcf, 0x33, 0xcf, 0x37, 0xee, 0x33, 0xee,
             // Channel 7
             0x3f, 0x8f, 0x3b, 0x8f, 0x3f, 0xaf, 0x3b, 0xaf,
             0x3f, 0xcf, 0x3b, 0xcf, 0x3f, 0xee, 0x3b, 0xee,
             // Channel 8
             0x47, 0x8f, 0x43, 0x8f, 0x47, 0xaf, 0x43, 0xaf,
             0x47, 0xcf, 0x43, 0xcf, 0x47, 0xee, 0x43, 0xee,
             // Channel 9
             0x4f, 0x8f, 0x4b, 0x8f, 0x4f, 0xaf, 0x4b, 0xaf,
             0x4f, 0xcf, 0x4b, 0xcf, 0x4f, 0xee, 0x4b, 0xee,
             0x6f, 0xed};

        for(int i=0; i<trig_count; i++) {
            sendPacket(fixed_packet);
        }
        */

    }
}

// Have to do this specialisation before instantiation in EmuController.h!

template<>
class EmuRxCore<StarChips> : virtual public RxCore {
        ClipBoard<RawData> m_queue;
        uint32_t m_channel = 0;
    public:
        EmuRxCore();
        ~EmuRxCore();
        
        void setCom(EmuCom *com) {} // Used by EmuController.h
        ClipBoard<RawData> &getCom() {return m_queue;}

        void setRxEnable(uint32_t val) override { m_channel = val; }
        void setRxEnable(std::vector<uint32_t> channels) override { if (not channels.empty()) m_channel = channels[0];}
        void maskRxEnable(uint32_t val, uint32_t mask) override {}
        void disableRx() override {}

        RawData* readData() override;
        
        uint32_t getDataRate() override {return 0;}
        uint32_t getCurCount() override {return m_queue.empty()?0:1;}
        bool isBridgeEmpty() override {return m_queue.empty();}
};


#include "EmuController.h"

template<class FE, class ChipEmu>
std::unique_ptr<HwController> makeEmu() {
  // This is just to match EmuController API, not used
  std::unique_ptr<RingBuffer> rx;
  std::unique_ptr<RingBuffer> tx(new RingBuffer(128));

  std::unique_ptr<HwController> ctrl(new EmuController<FE, ChipEmu>(std::move(rx), std::move(tx)));

  return ctrl;
}

EmuRxCore<StarChips>::EmuRxCore() {}
EmuRxCore<StarChips>::~EmuRxCore() {}

RawData* EmuRxCore<StarChips>::readData() {
    // //std::this_thread::sleep_for(std::chrono::microseconds(1));
    if(m_queue.empty()) return nullptr;

    std::unique_ptr<RawData> rd = m_queue.popData();
    // set rx channel number
    rd->adr = m_channel;

    return rd.release();
}

bool emu_registered_Emu =
  StdDict::registerHwController("emu_Star",
                                makeEmu<StarChips, StarEmu>);

template<>
void EmuController<StarChips, StarEmu>::loadConfig(json &j) {
  auto tx = EmuTxCore<StarChips>::getCom();
  auto &rx = EmuRxCore<StarChips>::getCom();

  //TODO make nice
  logger->info("-> Starting Emulator");
  std::string emuCfgFile;
  if (!j["feCfg"].empty()) {
    emuCfgFile = j["feCfg"];
    logger->info("Using config: {}", emuCfgFile);
  }

  // HPR packet:
  // 40000 BC (i.e. 1 ms) by default.
  // Can be set to a smaller value for testing, but need to be a multiple of 4
  unsigned hprperiod = 40000;
  if (!j["hprPeriod"].empty()) {
    hprperiod = j["hprPeriod"];
    logger->debug("HPR packet transmission period is set to {} BC", hprperiod);
  }

  emu.reset(new StarEmu( rx, tx_com.get(), emuCfgFile, hprperiod ));
  emuThreads.push_back(std::thread(&StarEmu::executeLoop, emu.get()));
}
