#ifndef RD53ACMD_H
#define RD53ACMD_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: Collection of FE-I4 commands
// ################################

#include <iostream>
#include "TxCore.h"

class Rd53aCmd {
    public:
        const static uint16_t enc5to8[32];
        static uint32_t encode5to8(uint32_t val); // custom 5b to 8b encoding

        // Slow Commands
        void globalPulse(uint32_t chipId, uint32_t duration);
        void cal(uint32_t chipId, uint32_t mode, uint32_t delay, uint32_t duration, uint32_t aux_mode=0, uint32_t aux_delay=0);
        void wrRegister(uint32_t chipId, uint32_t address, uint32_t value);
        void wrRegister(uint32_t chipId, uint32_t address, uint32_t values[3]);
        void rdRegister(uint32_t chipId, uint32_t address);

        // Fast Commands
        void trigger(uint32_t bc, uint32_t tag);
        void ecr();
        void bcr();

    protected:
        Rd53aCmd(TxCore *arg_core);
        ~Rd53aCmd();

        TxCore *core;
    private:
        bool verbose;
};

#endif

