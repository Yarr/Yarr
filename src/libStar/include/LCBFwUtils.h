#ifndef STAR_LCB_FW_HEADER
#define STAR_LCB_FW_HEADER

#include "TxCore.h"

#include <array>

// For the LCB encoder in FELIX Strips firmware
namespace LCB_FELIX {

  enum CmdType : uint8_t {
    BYPASS = 0x00,
    IDLE = 0x80,
    FASTCMD = 0x81,
    L0A = 0x82,
    ABCREGRD = 0xa0,
    HCCREGRD = 0xa1,
    ABCREGWR = 0xa2,
    HCCREGWR = 0xa3
  };

  inline std::array<uint8_t, 1> idle() {
    return {IDLE};
  }

  inline std::array<uint8_t, 2> fast_command(uint8_t cmdID, uint8_t delay) {
    return {
      FASTCMD,
      static_cast<uint8_t>( ((delay & 0x3) << 4) + (cmdID & 0xf) )
    };
  }

  inline std::array<uint8_t, 2> l0a_mask(uint8_t mask, bool bcr=false) { // tag?
    return {
      L0A,
      static_cast<uint8_t>( (bcr << 4) + (mask & 0xf) )
    };
  }

  inline std::array<uint8_t, 2> lonely_bcr() {
    return {L0A, 0x10};
  }

  inline std::array<uint8_t, 3> read_hcc_register(int address, int hccID = 0xf) {
    return {
      HCCREGRD,
      static_cast<uint8_t>( address & 0xff ),
      static_cast<uint8_t>( (hccID & 0xf) << 4 )
    };
  }

  inline std::array<uint8_t, 3> read_abc_register(int address, int hccID = 0xf, int abcID = 0xf) {
    return {
      ABCREGRD,
      static_cast<uint8_t>( address & 0xff ),
      static_cast<uint8_t>( ((hccID & 0xf) << 4) + (abcID & 0xf) )
    };
  }

  inline std::array<uint8_t, 7> write_hcc_register(int address, uint32_t value, int hccID = 0xf) {
    return {
      HCCREGWR,
      static_cast<uint8_t>( value >> 24 ),
      static_cast<uint8_t>( (value >> 16) & 0xff ),
      static_cast<uint8_t>( (value >> 8) & 0xff ),
      static_cast<uint8_t>( value & 0xff ),
      static_cast<uint8_t>( address & 0xff ),
      static_cast<uint8_t>( (hccID & 0xf) << 4 )
    };
  }

  inline std::array<uint8_t, 7> write_abc_register(int address, uint32_t value, int hccID = 0xf, int abcID = 0xf) {
    return {
      ABCREGWR,
      static_cast<uint8_t>( value >> 24 ),
      static_cast<uint8_t>( (value >> 16) & 0xff ),
      static_cast<uint8_t>( (value >> 8) & 0xff ),
      static_cast<uint8_t>( value & 0xff ),
      static_cast<uint8_t>( address & 0xff ),
      static_cast<uint8_t>( ((hccID & 0xf) << 4) + (abcID & 0xf) )
     };
  }

  // Configuration registers for the firmware LCB encoder
  enum ConfigReg {
    L0A_FRAME_PHASE = 0x00,
    L0A_FRAME_DELAY = 0x01,
    TTC_L0A_ENABLE = 0x02,
    TTC_BCR_DELAY = 0x03,
    GATING_TTC_ENABLE = 0x04,
    GATING_BC_START = 0x05,
    GATING_BC_STOP = 0x06,
    TRICKLE_TRIGGER_PULSE = 0x07,
    TRICKLE_TRIGGER_RUN = 0x08,
    TRICKLE_DATA_START = 0x09,
    TRICKLE_DATA_END = 0x0A,
    TRICKLE_WRITE_ADDR = 0x0B,
    TRICKLE_SET_WRITE_ADDR_PULSE = 0x0C,
    ENCODING_ENABLE = 0x0D,
    HCC_MASK = 0x0E,
    ABC_MASK_0 = 0x0F,
    ABC_MASK_1 = 0x10,
    ABC_MASK_2 = 0x11,
    ABC_MASK_3 = 0x12,
    ABC_MASK_4 = 0x13,
    ABC_MASK_5 = 0x14,
    ABC_MASK_6 = 0x15,
    ABC_MASK_7 = 0x16,
    ABC_MASK_8 = 0x17,
    ABC_MASK_9 = 0x18,
    ABC_MASK_A = 0x19,
    ABC_MASK_B = 0x1A,
    ABC_MASK_C = 0x1B,
    ABC_MASK_D = 0x1C,
    ABC_MASK_E = 0x1D,
    ABC_MASK_F = 0x1E
  };

  // Command to configure the firmware LCB encoder sent via config virtual elinks
  enum ConfigCmd : uint8_t {
    LCBREG = 0x10
  };

  inline std::array<uint8_t, 4> config_command_bytes(
    uint8_t addr, uint16_t value)
  {
    return {
      LCBREG,
      static_cast<uint8_t>( (value >> 8) & 0xff ),
      static_cast<uint8_t>( value & 0xff ),
      addr
    };
  }

  inline uint32_t config_command(uint8_t addr, uint16_t value) {
    std::array<uint8_t, 4> cmd = config_command_bytes(addr, value);
    return (cmd[0] << 24) + (cmd[1] << 16) + (cmd[2] << 8) + cmd[3];
  }

  void write_trickle_memory(
    TxCore& tx,
    const std::vector<uint32_t>& elinks_config,
    const std::vector<uint32_t>& elinks_trickle,
    const std::vector<uint8_t>& sequence
    )
  { 
    // Enable the LCB configuration elinks
    tx.setCmdEnable(elinks_config);

    // Set TRICKLE_WRITE_ADDR = 0
    tx.writeFifo(config_command(TRICKLE_WRITE_ADDR, 0));

    // Set TRICKLE_SET_WRITE_ADDR_PULSE = 1
    tx.writeFifo(config_command(TRICKLE_SET_WRITE_ADDR_PULSE, 1));

    // Set TRICKLE_DATA_START = 0
    tx.writeFifo(config_command(TRICKLE_DATA_START, 0));

    // Set TRICKLE_DATA_END to the number of bytes in sequence
    tx.writeFifo(config_command(TRICKLE_DATA_END, sequence.size()));

    tx.releaseFifo();

    while(!tx.isCmdEmpty());

    // Enable the trickle configuration elinks
    tx.setCmdEnable(elinks_trickle);

    // Write sequence to the trickle memory
    // Have to combine the bytes to 32-bit int
    for (unsigned iB=0; iB<sequence.size(); iB+=4) {
      uint32_t word(0);
      for (int offset=0; offset<4; offset++) {
        if (iB+offset < sequence.size())
          word += sequence[iB+offset] << ((3-offset)*8);
        else
          word += IDLE << ((3-offset)*8); // append IDLE
      }

      tx.writeFifo(word);
      tx.releaseFifo();
    }

    while(!tx.isCmdEmpty());

  } // write_trickle_memory

  const unsigned int TRICKLE_MEM_SIZE (1<<14); // Bytes

} // close namespace LCB_FELIX

#endif
