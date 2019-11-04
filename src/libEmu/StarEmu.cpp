#include "StarEmu.h"

#include <chrono>
#include <iomanip>
#include <fstream>
#include <thread>

#include "EmuCom.h"
#include "LCBUtils.h"

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

StarEmu::StarEmu(EmuCom * rx, EmuCom * tx, std::string json_file_path)
    : m_txRingBuffer ( tx )
    , m_rxRingBuffer ( rx )
{
    run = true;

    if (!json_file_path.empty()) {
      std::ifstream file(json_file_path);
      json j = json::parse(file);
      file.close();
    }
}

StarEmu::~StarEmu() {}

void StarEmu::sendPacket(uint8_t *byte_s, uint8_t *byte_e) {
    int byte_length = byte_e - byte_s;

    int word_length = (byte_length + 3) / 4;

    for(unsigned i=0; i<word_length-1; i++) {
        m_rxRingBuffer->write32(*(uint32_t*)&byte_s[i*4]);
    }

    if(byte_length%4) {
        uint32_t final = 0;
        for(unsigned i=0; i<byte_length%4; i++) {
            int offset = 8 * (i);
            final |= byte_s[(word_length-1)*4 + i] << offset;
        }
        m_rxRingBuffer->write32(final);
    }
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

bool emu_registered_Emu =
  StdDict::registerHwController("emu_Star",
                                makeEmu<StarChips, StarEmu>);

template<>
void EmuController<StarChips, StarEmu>::loadConfig(json &j) {
//    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
//    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));

  auto tx = EmuTxCore<StarChips>::getCom();
  auto rx = EmuRxCore<StarChips>::getCom();

  //TODO make nice
  std::cout << "-> Starting Emulator" << std::endl;
  std::string emuCfgFile;
  if (!j["feCfg"].empty()) {
    emuCfgFile = j["feCfg"];
    std::cout << " Using config: " << emuCfgFile << "\n";
  }
  emu.reset(new StarEmu( rx_com.get(), tx_com.get(), emuCfgFile ));
  emuThreads.push_back(std::thread(&StarEmu::executeLoop, emu.get()));
}
