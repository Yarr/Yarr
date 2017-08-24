// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A library
// # Comment: RD53A global register
// # Date: August 2017
// ################################

#include "Rd53aGlobalCfg.h"

Rd53aGlobalCfg::Rd53aGlobalCfg() {
    this->init();
}

Rd53aGlobalCfg::~Rd53aGlobalCfg() {
    
}

void Rd53aGlobalCfg::init() {
    for (unsigned int i=0; i<numRegs; i++)
        m_cfg[i] = 0x00;

    //0
    PixPortalHigh.init(&m_cfg[0], 8, 8, 0x0); regMap["PixPortalHigh"] = &PixPortalHigh; // TODO rename odd/even
    PixPortalLow.init(&m_cfg[0], 0, 8, 0x0);
    //1
    RegionCol.init(&m_cfg[1], 0, 8, 0x0);
    //2
    RegionRow.init(&m_cfg[2], 0, 9, 0x0);
    //3
    PixMode.init(&m_cfg[3], 4, 3, 0x0); // TODO need table in doc
    BMask.init(&m_cfg[3], 0, 3, 0x0);
    //4
    PixDefaultConfig.init(&m_cfg[4], 0, 16, 0x9CE2); // TODO why not the same
}
