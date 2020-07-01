// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Library
// # Comment: Collection of RD53B commands
// # Date: May 2020
// ################################

#include "Rd53bCmd.h"
#include "logging.h"

namespace {
  auto logger = logging::make_log("Rd53bCmd");
}

Rd53bCmd::Rd53bCmd() : core( nullptr ) {}

Rd53bCmd::Rd53bCmd(TxCore *arg_core) : core(arg_core) {}

Rd53bCmd::~Rd53bCmd() {}

constexpr uint16_t Rd53bCmd::enc5to8[];
constexpr uint16_t Rd53bCmd::encTrigger[];
constexpr uint16_t Rd53bCmd::encTag[];

std::array<uint16_t, 1> Rd53bCmd::genPllLock() {
    return {0xAA};
}
