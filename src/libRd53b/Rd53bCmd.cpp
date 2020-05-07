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
