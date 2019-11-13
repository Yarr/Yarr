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
{
    run = true;

    if (!json_file_path.empty()) {
      std::ifstream file(json_file_path);
      json j = json::parse(file);
      file.close();
    }

    /////
    m_ignoreCmd = true;
    m_isForABC = false;

    /////
    m_HCCID = 0;
    m_ABCIDs = {0};
    m_nABCs = m_ABCIDs.size();
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

void StarEmu::buildPhysicsPacket(PacketTypes typ, uint8_t l0tag,
                                 uint8_t bc_count, uint16_t endOfPacket)
{
    ///////////////////
    // Header: 16 bits
    bool errorflag = 0; // for now
    // BCID: lowest 3 bits of 8-bit  + 1 parity bit 
    bool bc_parity = getParity_8bits(bc_count);
    // packet type (4b) + flag error (1b) + L0tag (7b) + BCID (4b)
    uint16_t header = ((uint8_t)typ << 12) | errorflag << 11 | (l0tag & 0x7f) << 4 | (bc_count&7) << 1 | bc_parity;

    m_data_packets.push_back(header & 0xff00);
    m_data_packets.push_back(header & 0x00ff);
    
    ///////////////////
    // ABCStar clusters
    for (int ichannel=0; ichannel<m_clusters.size(); ++ichannel) {
        for ( uint16_t cluster : m_clusters[ichannel]) {
            // cluster bits:
            // "0" + 4-bit channel number + 11-bit cluster dropping the last cluster bit
            uint16_t clusterbits = (ichannel & 0xf)<<11 | (cluster & 0xfff)>>1;
            m_data_packets.push_back(clusterbits & 0xff00);
            m_data_packets.push_back(clusterbits & 0x00ff);
        }
    }

    // Todo: error block

    // Fixed 16-bit end of packet cluster pattern
    m_data_packets.push_back(endOfPacket & 0xff00);
    m_data_packets.push_back(endOfPacket & 0x00ff);
}

void StarEmu::buildABCRegisterPacket(
    PacketTypes typ, uint8_t input_channel, uint8_t reg_addr, unsigned reg_data,
    uint16_t reg_status)
{   
    // first byte: 4-bit type + 4-bit HCC input channel
    uint8_t byte1 = ((uint8_t)typ & 0xf ) << 4 | (input_channel & 0xf);
    m_data_packets.push_back(byte1);
    
    // then 8-bit register address
    m_data_packets.push_back(reg_addr);

    // 4-bit TBD + 32-bit data + 16-bit statis + '0000'
    m_data_packets.push_back(reg_data >> 28);
    m_data_packets.push_back((reg_data >> 20) & 0xff);
    m_data_packets.push_back((reg_data >> 12) & 0xff);
    m_data_packets.push_back((reg_data >> 4) & 0xff);
    m_data_packets.push_back((reg_data & 0xf) << 4);
}

void StarEmu::buildHCCRegisterPacket(
    PacketTypes typ, uint8_t reg_addr, unsigned reg_data, uint16_t reg_status)
{
    // 4-bit type + 8-bit register address + 32-bit data + '0000'
    m_data_packets.push_back( ((uint8_t)typ & 0xf) << 4 | (reg_addr >> 4) );
    m_data_packets.push_back( ((reg_addr & 0xf) << 4) | (reg_data >> 28) );
    m_data_packets.push_back((reg_data >> 20) & 0xff);
    m_data_packets.push_back((reg_data >> 12) & 0xff);
    m_data_packets.push_back((reg_data >> 4) & 0xff);
    m_data_packets.push_back((reg_data & 0xf) << 4);
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
            // do nothing
        }
    } // if (not (SixEight::iskcode(code0) or SixEight::iskcode(code1)) )
    
}

void StarEmu::doL0A(uint16_t data12) {
    bool bcr = (data12 >> 11) & 1;  // BC reset
    uint8_t l0a_mask = (data12 >> 7) & 0xf; // 4-bit L0A mask
    uint8_t l0a_tag = data12 & 0x7f; // 7-bit L0A tag
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
}

void StarEmu::doRegReadWrite(LCB::Frame frame) {
    uint8_t code0 = (frame >> 8) & 0xff;
    uint8_t code1 = frame & 0xff;

    if (code0 == LCB::K2) { // This frame is a K2 Start or K2 End 
        // Decode the second symbol
        uint8_t data6 = SixEight::decode(code1);

        m_isForABC = (data6 >> 5) & 1; // Otherwise it is a HCC command        
        bool isK2Start = (data6 >> 4) & 1; // Otherwise it is a K2 End
        unsigned cmd_hccID = data6 & 0xf; // Bottom 4 bits for HCC ID
        // Ignore the command sequence unless the HCC ID matches the ID on chip
        // or it is a broadcast command (0b1111)
        m_ignoreCmd = not ( cmd_hccID == (m_HCCID & 0xf) or cmd_hccID == 0xf);

        if (m_ignoreCmd) return;
        
        if (isK2Start) {
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

            // Get the header
            uint8_t header1 = m_reg_cmd_buffer.front();
            m_reg_cmd_buffer.pop();
            uint8_t header2 = m_reg_cmd_buffer.front();
            m_reg_cmd_buffer.pop();

            bool isRegRead = (header1 >> 6) & 1; // Otherwise write register
            unsigned cmd_abcID = (header1 >> 2) & 0xf;
            uint8_t reg_addr = ((header1 & 3) << 6) | ((header2 >> 1) & 0x3f); 

            // Access register
            if (isRegRead) { // read register
                // Check if K2 End occurs at the correct stage for reg read
                if (not m_reg_cmd_buffer.empty()) {
                    if (verbose) {
                        std::cout << __PRETTY_FUNCTION__ << " : K2 End occurs at the wrong stage for reading register!" << std::endl;
                    }
                    return;
                }

                // read
                readRegister(reg_addr, m_isForABC, cmd_abcID);
                
            }
            else { // write register
                // Check if K2 End occurs at the correct stage for reg write
                if (m_reg_cmd_buffer.size() != 5) {
                    if (verbose) {
                        std::cout << __PRETTY_FUNCTION__ << " :  K2 End occurs at the wrong stage for writing register!" << std::endl;
                    }
                    return;
                }

                uint32_t data = 0;
                for (int i = 4; i >= 0; --i) {
                    data |= ((m_reg_cmd_buffer.front() & 0x7f) << (7*i));
                    m_reg_cmd_buffer.pop();
                }

                // write
                writeRegister(data, reg_addr, m_isForABC, cmd_abcID);
            }
            
            assert(m_reg_cmd_buffer.empty());
        }  
    }
    else {
        if (m_ignoreCmd) return;
        
        // Decode the frame
        uint16_t data12 = (SixEight::decode(code0) << 6) | (SixEight::decode(code1));
        // Top 5 bits should be zeros
        assert(not (data12>>7));
        // Store it into the buffer. Only the lowest 7 bits is meaningful.
        m_reg_cmd_buffer.push(data12 & 0x7f);
    }
}

void StarEmu::writeRegister(const uint32_t data, const uint8_t address,
                            bool isABC, const unsigned ABCID)
{}

void StarEmu::readRegister(const uint8_t address, bool isABC,
                           const unsigned ABCID)
{}

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
