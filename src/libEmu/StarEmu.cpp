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
    }

    m_ignoreCmd = true;
    m_isForABC = false;

    // HCCStar and ABCStar configurations
    // TODO: should get these from chip config json file
    // m_starCfg->fromFileJson(j_starCfg);
    // for now
    m_starCfg->setHCCchipID(0);
    m_starCfg->m_nABC = 11;
    m_starCfg->setABCchipIDs();

    m_starCfg->initRegisterMaps();

    m_clusters.reserve(m_starCfg->m_nABC);
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

bool StarEmu::getParity_8bits(uint8_t val)
{
    val ^= val >> 4;
    val ^= val >> 2;
    val ^= val >> 1;
    return val&1;
}

std::vector<uint8_t> StarEmu::buildPhysicsPacket(
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
    for (int ichannel=0; ichannel<m_clusters.size(); ++ichannel) {
        for ( uint16_t cluster : m_clusters[ichannel]) {
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

    // 4-bit TBD + 32-bit data + 16-bit statis + '0000'
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

unsigned int countTriggers(LCB::Frame frame) {
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

void StarEmu::DecodeLCB(LCB::Frame frame) {
    // {code0, code1}
    uint8_t code0 = (frame >> 8) & 0xff;
    uint8_t code1 = frame & 0xff;

    bool iskcode0 = SixEight::iskcode(code0);
    bool iskcode1 = SixEight::iskcode(code1);

    if (not (SixEight::iskcode(code0) or SixEight::iskcode(code1)) ) {
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
            m_bccnt += 4;
            // do nothing
        }
    } // if (not (SixEight::iskcode(code0) or SixEight::iskcode(code1)) )
    
}

void StarEmu::doL0A(uint16_t data12) {
    bool bcr = (data12 >> 11) & 1;  // BC reset
    uint8_t l0a_mask = (data12 >> 7) & 0xf; // 4-bit L0A mask
    uint8_t l0a_tag = data12 & 0x7f; // 7-bit L0A tag

    // A LCB frame covers 4 BCs
    for (int ibc = 0; ibc < 4; ++ibc) {

        // check if there is a L0A
        // msb of L0A mask corresponds to the earliest BC
        if ( (l0a_mask >> (3-ibc)) & 1 ) {
            
            // get clusters for this BC
            getClusters();

            // build and send data packet
            PacketTypes ptype = PacketTypes::LP; // for now
            std::vector<uint8_t> packet = buildPhysicsPacket(ptype, l0a_tag, m_bccnt);
            sendPacket(packet);
        }
        
        m_bccnt += 1;
    }
    
    if (bcr) m_bccnt = 0;
}

void StarEmu::doFastCommand(uint8_t data6) {
    uint8_t bcsel = (data6 >> 4) & 3; // top 2 bits for BC select
    uint8_t fastcmd = data6 & 0xf; // bottom 4 bits for command

    switch((StarCmd::FastCommands)fastcmd) {
    case StarCmd::FastCommands::LogicReset :
        std::cout << "Fast command: LogicReset" << std::endl;
        break;
    case StarCmd::FastCommands::ABCRegReset :
        std::cout << "Fast command: ABCRegReset" << std::endl;
        break;
    case StarCmd::FastCommands::ABCSEUReset :
        std::cout << "Fast command: ABCSEUReset" << std::endl;
        break;
    case StarCmd::FastCommands::ABCCaliPulse :
        std::cout << "Fast command: ABCCaliPulse" << std::endl;
        break;
    case StarCmd::FastCommands::ABCDigiPulse :
        std::cout << "Fast command: ABCDigiPulse" << std::endl;
        break;
    case StarCmd::FastCommands::ABCHitCntReset :
        std::cout << "Fast command: ABCHitCntReset" << std::endl;
        break;
    case StarCmd::FastCommands::ABCHitCntStart :
        std::cout << "Fast command: ABCHitCntStart" << std::endl;
        break;
    case StarCmd::FastCommands::ABCHitCntStop :
        std::cout << "Fast command: ABCHitCntStop" << std::endl;
        break;
    case StarCmd::FastCommands::ABCSlowCmdReset :
        std::cout << "Fast command: ABCSlowCmdReset" << std::endl;
        break;
    case StarCmd::FastCommands::StopPRLP :
        std::cout << "Fast command: StopPRLP" << std::endl;
        break;
    case StarCmd::FastCommands::HCCRegReset :
        std::cout << "Fast command: HCCRegReset" << std::endl;
        break;
    case StarCmd::FastCommands::HCCSEUReset :
        std::cout << "Fast command: HCCSEUReset" << std::endl;
        break;
    case StarCmd::FastCommands::HCCPLLReset :
        std::cout << "Fast command: HCCPLLReset" << std::endl;
        break;
    case StarCmd::FastCommands::StartPRLP :
        std::cout << "Fast command: StartPRLP" << std::endl;
        break;
    }

    m_bccnt += 4;
}

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
        // Store it into the buffer. Only the lowest 7 bits is meaningful.
        m_reg_cmd_buffer.push(data12 & 0x7f);
    }

    // Increment BC counter
    m_bccnt += 4;
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
            for (int index=1; index <= m_starCfg->m_nABC; ++index)
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

        // write register
        // If cmd_abcID is '1111' i.e. broadcast address, write all ABCs
        if ((cmd_abcID & 0xf) == 0xf and m_isForABC) {
            for (int index=1; index <= m_starCfg->m_nABC; ++index)
                writeRegister(data, reg_addr, true, m_starCfg->getABCchipID(index));
        } else {
            writeRegister(data, reg_addr, m_isForABC, cmd_abcID);
        }
        assert(m_reg_cmd_buffer.empty());
    } // if (isRegRead)
}

std::array<unsigned, 8> StarEmu::getFrontEndData(unsigned chipID,
                                                 uint8_t bc_index)
{
    std::array<unsigned, 8> inputs;

    // test mode
    uint8_t TM = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, chipID, 17, 16);
    bool testPulseEnable = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, chipID, 4, 4);
    
    // mask registers
    auto maskinput0 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput0, chipID);
    auto maskinput1 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput1, chipID);
    auto maskinput2 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput2, chipID);
    auto maskinput3 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput3, chipID);
    auto maskinput4 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput4, chipID);
    auto maskinput5 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput5, chipID);
    auto maskinput6 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput6, chipID);
    auto maskinput7 = m_starCfg->getABCRegister(emu::ABCStarRegs::MaskInput7, chipID);
    
    if (TM == 0) { // Normal Data Taking
        // Eight 32-bit integers for 256 strips
        // A fixed pattern for now
        inputs[0] = (~maskinput7) & 0xfffe0000; // ch255 - 224
        inputs[1] = (~maskinput6) & 0x0;        // ch223 - 192
        inputs[2] = (~maskinput5) & 0x0;        // ch191 - 160
        inputs[3] = (~maskinput4) & 0x0;        // ch159 - 128
        inputs[4] = (~maskinput3) & 0xfffe0000; // ch127 - 96
        inputs[5] = (~maskinput2) & 0x0;        // ch95 - 64
        inputs[6] = (~maskinput1) & 0x0;        // ch63 - 32
        inputs[7] = (~maskinput0) & 0x0;        // ch31 - 0
        
        // Expected clusterFinder output:
        // {0x78f, 0x38f, 0x7af, 0x3af, 0x7cf, 0x3cf, 0x7ee, 0xbee};
    }
    else if (TM == 1) { // Static Test Mode
        inputs = {maskinput7, maskinput6, maskinput5, maskinput4,
                  maskinput3, maskinput2, maskinput1, maskinput0};
    }
    else if (TM == 2 and testPulseEnable) { // Test Pulse Mode
        uint8_t testPatt1 = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, chipID, 23, 20);
        uint8_t testPatt2 = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, chipID, 27, 24);
        bool testPattEnable = m_starCfg->getABCSubRegValue(emu::ABCStarRegs::CREG0, chipID, 18, 18);
        
        if (testPattEnable) {
            // Test Pattern
            // If mask bit is 1, use bc_index'th bit of testPatt1
            // Otherwise, use bc_index'th bit of testPatt2
            bool patt1_i = ((testPatt1 >> bc_index) & 1) ? 0xffffffff : 0;
            bool patt2_i = ((testPatt2 >> bc_index) & 1) ? 0xffffffff : 0;

            inputs[0] = maskinput7 & patt1_i | ~maskinput7 & patt2_i;
            inputs[1] = maskinput6 & patt1_i | ~maskinput6 & patt2_i;
            inputs[2] = maskinput5 & patt1_i | ~maskinput5 & patt2_i;
            inputs[3] = maskinput4 & patt1_i | ~maskinput4 & patt2_i;
            inputs[4] = maskinput3 & patt1_i | ~maskinput3 & patt2_i;
            inputs[5] = maskinput2 & patt1_i | ~maskinput2 & patt2_i;
            inputs[6] = maskinput1 & patt1_i | ~maskinput1 & patt2_i;
            inputs[7] = maskinput0 & patt1_i | ~maskinput0 & patt2_i;
        }
        else {
            // Get the Mask bit value for one clock
            // Should be triggered by the "Digital Test Pulse" fast command
            inputs = {maskinput7, maskinput6, maskinput5, maskinput4,
                      maskinput3, maskinput2, maskinput1, maskinput0};
        }
    }

    return inputs;
}

std::vector<uint16_t> StarEmu::clusterFinder(
    const std::array<unsigned, 8>& inputData, const uint8_t maxCluster)
{
    // inputData consists of eight 32-bit data registers for 256 strips
    // The bit order of these data registers are assumed to be consist with
    // the channel mask registers (Table Table 9-30 of ABCStar Spec v7.80)
    // The 256 strips are divided into two rows to form clusters
    // {inputData[4],inputData[5],inputData[6],inputData[7]}: channel 127 - 0
    // {inputData[0],inputData[1],inputData[2],inputData[3]}: channel 255 - 128

    std::vector<uint16_t> clusters;

    // combine input data into uint64_t
    uint64_t d0l = (uint64_t)inputData[6] << 32 | inputData[7];
    uint64_t d0h = (uint64_t)inputData[4] << 32 | inputData[5];
    uint64_t d1l = (uint64_t)inputData[2] << 32 | inputData[3];
    uint64_t d1h = (uint64_t)inputData[0] << 32 | inputData[1];

    while (d0l or d0h or d1l or d1h) {
        if (clusters.size() >= maxCluster) break;

        uint16_t cluster1 = clusterFinder_sub(d1h, d1l, true);
        if (cluster1 != 0x3fe) // if a non-empty cluster is found
            clusters.push_back(cluster1);

        if (clusters.size() >= maxCluster)  break;

        uint16_t cluster0 = clusterFinder_sub(d0h, d0l, false);
        if (cluster0 != 0x3fe) // if a non-empty cluster is found
            clusters.push_back(cluster0);
    }

    // last cluster
    clusters.back() |= 1 << 11;

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
        return 0x3fe;
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

void StarEmu::getClusters()
{
    m_clusters.clear();
    /*
    // Fixed cluster pattern for now
    std::vector<uint16_t> a_fixed_cluster_pattern =
        {0x78f, 0x38f, 0x7af, 0x3af, 0x7cf, 0x3cf, 0x7ee, 0xbee};
    */

    // Get frontend data and find clusters
    for (int index=1; index <= m_starCfg->m_nABC; ++index) {
        unsigned abcID = m_starCfg->getABCchipID(index);
        std::vector<uint16_t> a_cluster = clusterFinder(getFrontEndData(abcID));
        m_clusters.push_back(a_cluster);
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
        if (ich >= m_starCfg->m_nABC) {
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
    public:
        EmuRxCore();
        ~EmuRxCore();
        
        void setCom(EmuCom *com) {} // Used by EmuController.h
        ClipBoard<RawData> &getCom() {return m_queue;}

        void setRxEnable(uint32_t val) override {}
        void setRxEnable(std::vector<uint32_t> channels) override {}
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

    return m_queue.popData().release();
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
