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

  /// General pattern for all control and status registers commands
  std::array<LCB::Frame, 9> command_sequence(int hccID = 0xf, int chipID = 0xf,
                                             int address = 0, bool readNotWrite = false, uint32_t value = 0, bool hccNotAbc = false);

  /// Write HCC register, defaults to broacast
  std::array<LCB::Frame, 9> write_hcc_register(int address, uint32_t value, int hccID = 0xf) {
    return command_sequence(hccID, 0xf, address, false, value, true);
  }

  /// Write ABC register, defaults to broacast
  std::array<LCB::Frame, 9> write_abc_register(int address, uint32_t value, int hccID = 0xf, int abcID = 0xf) {
    return command_sequence(hccID, abcID, address, false, value, false);
  }

  /// Read HCC register, defaults to broacast
  std::array<LCB::Frame, 9> read_hcc_register(int address, int hccID = 0xf) {
    return command_sequence(hccID, 0xf, address, true, 0, true);
  }

  /// Read ABC register, defaults to broacast
  std::array<LCB::Frame, 9> read_abc_register(int address, int hccID = 0xf, int abcID = 0xf) {
    return command_sequence(hccID, abcID, address, true, 0, false);
  }
  
};

#endif
