#ifndef STAR_CMD_H
#define STAR_CMD_H

// #################################
// # Project: Yarr
// # Description: Star commands
// ################################

#include <array>

#include "LCBUtils.h"

/**
 * Build command sequences for Star chips.
 *
 * Utility class, used by super-classes.
 */
class StarCmd {
 public:
  StarCmd() = default;
  ~StarCmd() = default;

  // Access Control and Status Registers (ACSR) Commands
  std::array<LCB::Frame, 9> command_sequence(int hccID = 0xf, int chipID = 0xf,
                                             int address = 0, bool readNotWrite = false, uint32_t value = 0, bool hccNotAbc = false);

  std::array<LCB::Frame, 9> write_hcc_register(int address, uint32_t value, int hccID = 0xf) {
    return command_sequence(hccID, 0xf, address, false, value, true);
  }

  std::array<LCB::Frame, 9> write_abc_register(int address, uint32_t value, int hccID = 0xf, int abcID = 0xf) {
    return command_sequence(hccID, abcID, address, false, value, false);
  }

  std::array<LCB::Frame, 9> read_hcc_register(int address, int hccID = 0xf) {
    return command_sequence(hccID, 0xf, address, true, 0, true);
  }

  std::array<LCB::Frame, 9> read_abc_register(int address, int hccID = 0xf, int abcID = 0xf) {
    return command_sequence(hccID, abcID, address, true, 0, false);
  }
  
};

#endif
