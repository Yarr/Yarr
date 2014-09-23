// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Base class
// ################################

#include "Fei4.h"

Fei4::Fei4(TxCore *core, unsigned channel, unsigned arg_chipId) : Fei4GlobalCfg(), Fei4Cmd(core, channel) {
    chipId = arg_chipId;
}

void Fei4::sendConfig() {
    runMode(false, chipId);
    
    // Increase threshold
    uint16_t tmp = getValue(&Fei4::Vthin_Coarse);
    writeRegister(&Fei4::Vthin_Coarse, 255);

    for (unsigned i=0; i<numRegs; i++) {
        wrRegister(chipId, i, cfg[i]);
    }

    // Set actual threshold
    setValue(&Fei4::Vthin_Coarse, tmp);
    writeRegister(&Fei4::Vthin_Coarse);

}

