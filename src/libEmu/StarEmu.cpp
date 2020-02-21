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

}

StarEmu::StarEmu(ClipBoard<RawData> &rx, EmuCom * tx, std::string json_file_path)
    : m_txRingBuffer ( tx )
    , m_rxQueue ( rx )
    , m_bccnt( 0 )
    , m_starCfg( new emu::StarCfg )
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

    hpr_clkcnt = HPRPERIOD/2; // 20000 BCs or 500 us
    hpr_sent.resize(m_starCfg->nABCs() + 1); // 1 HCCStar + nABCs ABCStar chips
    std::fill(hpr_sent.begin(), hpr_sent.end(), false);
    
    // HCCStar and ABCStar configurations
    // TODO: should get these from chip config json file
    // m_starCfg->fromFileJson(j_starCfg);
    // for now
    m_starCfg->init();

    m_l0buffers_lite.clear();
    m_l0buffers_lite.resize(m_starCfg->nABCs());

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
            if (cluster == 0x3fe) // "no cluster byte"
                break;

            // cluster bits:
            // "0" + 4-bit channel number + 11-bit cluster dropping the last cluster bit
            uint16_t clusterbits = (ichannel & 0xf)<<11 | cluster & 0x7ff;
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
    data_packets.push_back((reg_data & 0xf) << 4);

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

    // HPR
    doHPR(frame);

    // {code0, code1}
    uint8_t code0 = (frame >> 8) & 0xff;
    uint8_t code1 = frame & 0xff;

    bool iskcode0 = SixEight::iskcode(code0);
    bool iskcode1 = SixEight::iskcode(code1);

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
            if (verbose) std::cout << __PRETTY_FUNCTION__ << " : received an IDLE frame" << std::endl;
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
                if (verbose) {
                    std::cout << __PRETTY_FUNCTION__
                              << " : K2 End occurs at the wrong stage! "
                              << "Current command sequence size (excluding K2 frames): "
                              << m_reg_cmd_buffer.size() << std::endl;
                }
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
        unsigned ich = m_starCfg->indexForABCchipID(ABCID) - 1;
        if (ich >= m_starCfg->nABCs()) {
            std::cout << __PRETTY_FUNCTION__ << ": Cannot find an ABCStar chip with ID = " << ABCID << std::endl;
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
        if (debug) {
            std::cout << "StarEmu : received a register read command -> ";
            std::cout << "addr = " << (int)reg_addr << " isABC = " << m_isForABC;
            std::cout << " abcID = " << cmd_abcID << std::endl;
        }
        /*
        // Check if K2 End occurs at the correct stage for reg read
        if (not m_reg_cmd_buffer.empty()) {
            if (verbose) {
                std::cout << __PRETTY_FUNCTION__ << " : K2 End occurs at the wrong stage for reading register!" << std::endl;
            }
            return;
        }
        */
        // If cmd_abcID is '1111' i.e. broadcast address, read all ABCs
        if ((cmd_abcID & 0xf) == 0xf and m_isForABC) {
            for (int index=1; index <= m_starCfg->nABCs(); ++index)
                readRegister(reg_addr, true, m_starCfg->getABCchipID(index));
        } else {
            readRegister(reg_addr, m_isForABC, cmd_abcID);
        }
    } else { // register write command
        // Check if K2 End occurs at the correct stage for reg write
        if (m_reg_cmd_buffer.size() != 5) {
            if (verbose)
                std::cout << __PRETTY_FUNCTION__ << " :  K2 End occurs at the wrong stage for writing register!" << std::endl;
            return;
        }

        uint32_t data = 0;
        for (int i = 4; i >= 0; --i) {
            data |= ((m_reg_cmd_buffer.front() & 0x7f) << (7*i));
            m_reg_cmd_buffer.pop();
        }

        if (debug) {
            std::cout << "StarEmu : received a register write command -> ";
            std::cout << "addr = " << (int)reg_addr << " data = " << data;
            std::cout << " isABC = " << m_isForABC << " abcID = " << cmd_abcID << std::endl;
        }
        
        // write register
        // If cmd_abcID is '1111' i.e. broadcast address, write all ABCs
        if ((cmd_abcID & 0xf) == 0xf and m_isForABC) {
            for (int index=1; index <= m_starCfg->nABCs(); ++index)
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
    
    // Reset commands reset everything at once, ignoring the selected BC for now
    switch(fastcmd) {
    case LCB::LOGIC_RESET :
        this->logicReset();
        break;
    case LCB::ABC_REG_RESET :
        m_starCfg->resetABCRegisters();
        this->logicReset();
        break;
    case LCB::ABC_SEU_RESET :
        m_starCfg->resetABCSEU();
        break;
    case LCB::ABC_CAL_PULSE :
        for (int ichip=1; ichip <= m_starCfg->nABCs(); ++ichip) {
            this->generateFEData_CaliPulse(ichip, bcsel);
        }
        break;
    case LCB::ABC_DIGITAL_PULSE :
        for (int ichip=1; ichip <= m_starCfg->nABCs(); ++ichip) {
            this->generateFEData_TestPulse(ichip, bcsel);
        }
        break;
    case LCB::ABC_HIT_COUNT_RESET :
        m_starCfg->resetABCHitCounts();
        break;
    case LCB::ABC_HITCOUNT_START :
        m_startHitCount = true;
        break;
    case LCB::ABC_HITCOUNT_STOP :
        m_startHitCount = false;
        break;
    case LCB::ABC_SLOW_COMMAND_RESET :
        this->slowCommandReset();
        break;
    case LCB::ABC_STOP_PRLP :
        std::cout << "Fast command: StopPRLP" << std::endl;
        break;
    case LCB::HCC_REG_RESET :
        m_starCfg->resetHCCRegisters();
        //hpr_clkcnt = HPRPERIOD/2;
        //std::fill(hpr_sent.begin(), hpr_sent.end(), false);
        break;
    case LCB::HCC_SEU_RESET :
        m_starCfg->resetHCCSEU();
        break;
    case LCB::HCC_PLL_RESET :
        m_starCfg->resetHCCPLL();
        break;
    case LCB::ABC_START_PRLP :
        std::cout << "Fast command: StartPRLP" << std::endl;
        break;
    }
}

void StarEmu::logicReset()
{
    m_bccnt = 0;
    hpr_clkcnt = HPRPERIOD/2;
    std::fill(hpr_sent.begin(), hpr_sent.end(), false);
    
    for (unsigned ichip = 1; ichip <= m_starCfg->nABCs(); ++ichip) {
        clearFEData(ichip);
    }
}

void StarEmu::slowCommandReset()
{
    m_ignoreCmd = true;
    m_isForABC = false;

    // clear command buffer
    std::queue<uint8_t> empty_buffer;
    std::swap(m_reg_cmd_buffer, empty_buffer);
}

//
// HPR
//
void StarEmu::doHPR(LCB::Frame frame)
{
    doHPR_HCC(frame);

    for (unsigned ichip = 1; ichip <= m_starCfg->nABCs(); ++ichip) {
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
    bool testHPR = m_starCfg->getHCCSubRegValue(emu::HCCStarRegister::Pulse, 1, 1);
    bool stopHPR = m_starCfg->getHCCSubRegValue(emu::HCCStarRegister::Pulse, 0, 0);
    bool maskHPR = m_starCfg->getHCCSubRegValue(emu::HCCStarRegister::Cfg1, 8, 8);

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
            (uint8_t)emu::HCCStarRegister::HPR,
            m_starCfg->getHCCRegister(emu::HCCStarRegister::HPR));

        sendPacket(packet_hcchpr);

        hpr_sent[0] = true;
    }

    //// Update HPR control bits
    // Reset stopHPR to zero (i.e. resume the periodic HPR packet transmission)
    // if lcb_lock_changed
    if (stopHPR and lcb_lock_changed)
        m_starCfg->setHCCSubRegister(emu::HCCStarRegister::Pulse, 0, 0, 0);

    // Reset testHPR bit to zero if it is one
    if (testHPR)
        m_starCfg->setHCCSubRegister(emu::HCCStarRegister::Pulse, 0, 1, 1);
}

void StarEmu::doHPR_ABC(LCB::Frame frame, unsigned ichip)
{
    int abcID = m_starCfg->getABCchipID(ichip);

    //// Update the HPR register
    setABCStarHPR(frame, abcID);

    //// HPR control logic
    bool testHPR = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::SCReg, abcID, 3, 3);
    bool stopHPR = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::SCReg, abcID, 2, 2);
    bool maskHPR = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 6, 6);

    bool lcb_lock_changed = testHPR & (~maskHPR);
    bool hpr_periodic = not (hpr_clkcnt%HPRPERIOD) and not stopHPR;
    bool hpr_initial = not hpr_sent[ichip] and stopHPR;

    //// Build and send HPR packets
    if (lcb_lock_changed or hpr_periodic or hpr_initial) {
        auto packet_abchpr = buildABCRegisterPacket(
            PacketTypes::ABCHPR, ichip-1, (uint8_t)emu::ABCStarRegs::HPR,
            m_starCfg->getABCRegister(emu::ABCStarRegs::HPR, abcID),
            (abcID&0xf) << 12);

        sendPacket(packet_abchpr);

        hpr_sent[ichip] = true;
    }

    //// Update HPR control bits
    if (stopHPR and lcb_lock_changed)
        m_starCfg->setABCSubRegister(emu::ABCStarRegs::SCReg, 0, abcID, 2, 2);
    if (testHPR)
        m_starCfg->setABCSubRegister(emu::ABCStarRegs::SCReg, 0, abcID, 3, 3);
}

void StarEmu::setHCCStarHPR(LCB::Frame frame)
{
    uint32_t hprWord = frame << 16; // TODO: set other bits
    m_starCfg->setHCCRegister(emu::HCCStarRegister::HPR, hprWord);
}

void StarEmu::setABCStarHPR(LCB::Frame frame, int abcID)
{
    uint32_t hprWord = frame << 16; // TODO: set other bits
    m_starCfg->setABCRegister(emu::ABCStarRegs::HPR, hprWord, abcID);
}

//
// Trigger and front-end data
//
void StarEmu::doL0A(uint16_t data12) {
    bool bcr = (data12 >> 11) & 1;  // BC reset
    uint8_t l0a_mask = (data12 >> 7) & 0xf; // 4-bit L0A mask
    uint8_t l0a_tag = data12 & 0x7f; // 7-bit L0A tag

    // Prepare front-end data
    // Loop over all ABCStar chips
    for (int ichip = 1; ichip <= m_starCfg->nABCs(); ++ichip) 
        prepareFEData(ichip);
    
    // A LCB frame covers 4 BCs
    for (unsigned ibc = 0; ibc < 4; ++ibc) {
        // check if there is a L0A
        // msb of L0A mask corresponds to the earliest BC
        if ( (l0a_mask >> (3-ibc)) & 1 ) {
            // clusters to be sent
            std::vector<std::vector<uint16_t>> clusters;
            
            // Read all ABCStar chips
            for (unsigned iabc = 0; iabc < m_starCfg->nABCs(); ++iabc) {
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
    
    int abcID = m_starCfg->getABCchipID(iABC+1);
    bool EnCount = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 5, 5);
    if (not EnCount) return;

    auto feBC = getL0BufferAddr(iABC, cmdBC);

    // HitCountReg0-63: four channels per register
    for (int ireg = 0; ireg < 64; ++ireg) {
        // Read HitCount Register
        // address
        unsigned addr = ireg + (unsigned)emu::ABCStarRegs::HitCountREG0;
        // value
        unsigned counts = m_starCfg->getABCRegister(addr, abcID);
        
        // Compute increments that should be added to the current counts
        unsigned incr = 0;

        // Four front-end channels: [ireg*4], [ireg*4+1], [ireg*4+2], [ireg*4+3]
        for (int ich = 0; ich < 4; ++ich) {
            // check if the counts for this channel has already reached maximum
            if ( (counts>>(8*ich)) & 0xff == 0xff ) continue;

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
    unsigned maskinput0 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput0, abcID);
    unsigned maskinput1 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput1, abcID);
    unsigned maskinput2 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput2, abcID);
    unsigned maskinput3 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput3, abcID);
    unsigned maskinput4 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput4, abcID);
    unsigned maskinput5 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput5, abcID);
    unsigned maskinput6 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput6, abcID);
    unsigned maskinput7 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput7, abcID);

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

    int abcID = m_starCfg->getABCchipID(ichip);
    unsigned iABC = ichip - 1;

    uint8_t TM = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 17, 16);
    if (TM != 0) // do nothing if not normal data taking mode
        return;

    StripData masks = getMasks(ichip);

    for (int ibc = 0; ibc < 4; ++ibc)
        m_l0buffers_lite[iABC][ibc] &= masks.flip();
}

void StarEmu::generateFEData_StaticTest(unsigned ichip)
{
    int abcID = m_starCfg->getABCchipID(ichip);

    // ABC index starts from 1. Zero is reserved for HCC.
    unsigned iABC = ichip - 1;

    uint8_t TM = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 17, 16);
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

    int abcID = m_starCfg->getABCchipID(ichip);

    // ABC index starts from 1. Zero is reserved for HCC.
    unsigned iABC = ichip - 1;

    uint8_t TM = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 17, 16);
    if (TM != 2) // do nothing if not test pulse mode
        return;
    
    // enable
    bool TestPulseEnable = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 4, 4);
    if (not TestPulseEnable)
        return;

    StripData masks = getMasks(ichip);

    // Two test pulse options: determined by bit 18 of ABC register CREG0
    bool testPattEnable = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 18, 18);
    
    if (testPattEnable) { // Use test pattern for four consecutive BC
        // Test patterns
        // testPatt1 if mask bit is 0, otherwise testPatt2
        uint8_t testPatt1 = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 23, 20);
        uint8_t testPatt2 = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 27, 24);

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

    int abcID = m_starCfg->getABCchipID(ichip);

    // ABC index starts from 1. Zero is reserved for HCC.
    unsigned iABC = ichip - 1;

    uint8_t TM = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 17, 16);
    if (TM != 0) // do nothing if not normal data taking mode
        return;
    
    bool CalPulseEnable = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG1, abcID, 4, 4);
    if (not CalPulseEnable)
        return;
    
    // Injected charge DAC
    // 9 bits, 0 - 170 mV
    uint16_t BCAL = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::ADCS3, abcID, 24, 16);

    // Threshold DAC
    // BVT: 8 bits, 0 - -550 mV
    uint8_t BVT = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::ADCS2, abcID, 7, 0);
    // Trim Range
    // BTRANGE: 5 bits, 50 mV - 230 mV
    uint8_t BTRANGE = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::ADCS1, abcID, 28, 24);

    // Loop over 256 strips
    for (int istrip = 0; istrip < 256; ++istrip) {
        // TrimDAC
        uint8_t TrimDAC = m_starCfg->getTrimDAC(istrip, abcID);

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
    unsigned calenable0 = m_starCfg->getABCRegister(emu::ABCStarRegs::CalREG0, abcID);
    unsigned calenable1 = m_starCfg->getABCRegister(emu::ABCStarRegs::CalREG1, abcID);
    unsigned calenable2 = m_starCfg->getABCRegister(emu::ABCStarRegs::CalREG2, abcID);
    unsigned calenable3 = m_starCfg->getABCRegister(emu::ABCStarRegs::CalREG3, abcID);
    unsigned calenable4 = m_starCfg->getABCRegister(emu::ABCStarRegs::CalREG4, abcID);
    unsigned calenable5 = m_starCfg->getABCRegister(emu::ABCStarRegs::CalREG5, abcID);
    unsigned calenable6 = m_starCfg->getABCRegister(emu::ABCStarRegs::CalREG6, abcID);
    unsigned calenable7 = m_starCfg->getABCRegister(emu::ABCStarRegs::CalREG7, abcID);
    
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
    
    int abcID = m_starCfg->getABCchipID(ichip);

    // Check mode of operation
    uint8_t TM = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, abcID, 17, 16);
    
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
        data_low64 = (data_low64 & ~(1ULL << bit_addr)) | (value << bit_addr);
    }
    else if (bit_addr < 128) {
        data_high64 =
            (data_high64 & ~(1ULL << (bit_addr-64))) | (value << (bit_addr-64));
    }
}

void StarEmu::addClusters(std::vector<std::vector<uint16_t>>& allclusters,
                          unsigned iABC, uint8_t cmdBC)
{
    // Need to correct the BC from the L0A command by the L0A latency in order to
    // access the corresponding entries from m_l0buffers_lite[iABC] ring buffer
    auto feBC = getL0BufferAddr(iABC, cmdBC);

    // max clusters
    int abcID = m_starCfg->getABCchipID(iABC+1);
    bool maxcluster_en = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG3, abcID, 18, 18);
    uint8_t maxcluster = maxcluster_en ? m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG3, abcID, 17, 12) : 64;
    
    // Get front-end data and find clusters
    std::vector<uint16_t> abc_clusters = clusterFinder(
        m_l0buffers_lite[iABC][feBC], maxcluster);
    
    allclusters.push_back(abc_clusters);
}

unsigned StarEmu::getL0BufferAddr(unsigned iABC, uint8_t cmdBC)
{
    // cmdBC = 0, 1, 2, or 3 from L0A command
    
    // get L0A latency from the ABCStar config register CREG2
    int abcID = m_starCfg->getABCchipID(iABC+1);
    unsigned l0a_latency = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG2, abcID, 8, 0); // 9 bits, in unit of BC (25 ns)

    // return address of the FE data in m_l0buffers_lite[iABC] that corresponds to cmdBC
    // m_l0buffers_lite[iABC] is of size 4 and is used as a ring buffer
    // 512 ensures value in the bracket is positive 
    return (512 + cmdBC - l0a_latency) % 4;
}

void StarEmu::executeLoop() {
    std::cout << "Starting emulator loop" << std::endl;

    static const auto SLEEP_TIME = std::chrono::milliseconds(1);

    while (run) {
        if ( m_txRingBuffer->isEmpty()) {
            std::this_thread::sleep_for( SLEEP_TIME );
            continue;
        }

        if( verbose ) std::cout << __PRETTY_FUNCTION__ << ": -----------------------------------------------------------" << std::endl;

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
        std::vector<uint32_t> m_channels;
    public:
        EmuRxCore();
        ~EmuRxCore();
        
        void setCom(EmuCom *com) {} // Used by EmuController.h
        ClipBoard<RawData> &getCom() {return m_queue;}

        void setRxEnable(uint32_t val) override { m_channels.push_back(val); }
        void setRxEnable(std::vector<uint32_t> channels) override { m_channels = channels;}
        void maskRxEnable(uint32_t val, uint32_t mask) override {}

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
    if (not m_channels.empty()) rd->adr = m_channels[0];

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
  std::cout << "-> Starting Emulator" << std::endl;
  std::string emuCfgFile;
  if (!j["feCfg"].empty()) {
    emuCfgFile = j["feCfg"];
    std::cout << " Using config: " << emuCfgFile << "\n";
  }
  emu.reset(new StarEmu( rx, tx_com.get(), emuCfgFile ));
  emuThreads.push_back(std::thread(&StarEmu::executeLoop, emu.get()));
}
