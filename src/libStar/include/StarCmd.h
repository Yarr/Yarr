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
  StarCmd();
  ~StarCmd();

  // Access Control and Status Registers (ACSR) Commands
  std::array<LCB::Frame, 9> command_sequence(int hccID = 0xf, int chipID = 0x1f,
                                             int address = 0, bool write = false, uint32_t value = 0, bool hccNotAbc = false);
};

#endif
