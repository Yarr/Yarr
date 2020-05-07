// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B library
// # Comment: RD53B global register
// # Date: May 2020
// ################################

#include "Rd53bGlobalCfg.h"

#include "logging.h"

namespace {
    auto logger = logging::make_log("Rd53bGlobalCfg");
}

Rd53bGlobalCfg::Rd53bGlobalCfg() {

}

Rd53bGlobalCfg::~Rd53bGlobalCfg() {

}

void Rd53bGlobalCfg::init() {
    // Reset array
    for (unsigned i=0; i<numRegs; i++) {
        m_cfg[i] = 0x00;
    }

    //0

}
