#ifndef FEI4CMD_H
#define FEI4CMD_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: Collection of FE-I4 commands
// ################################

#include <iostream>
#include "TxCore.h"

class Fei4Cmd {
    protected:
        Fei4Cmd();
        Fei4Cmd(TxCore *arg_core);
        ~Fei4Cmd();

        void setCore(TxCore *arg_core);

        void setVerbose(bool v=true) {
            verbose = v;
        }
        
        // Fast Commands
        void trigger();
        void bcr();
        void ecr();
        void cal();

        // Slow Commdans
        void wrRegister(int chipId, int address, int value);
        void rdRegister(int chipId, int address);
        void wrFrontEnd(int chipId, uint32_t *bitstream);
        void runMode(int chipId, bool mode);
        void globalReset(int chipId);
        void globalPulse(int chipId, unsigned width);

        void calTrigger(int delay);

        TxCore *core;
    private:
        bool verbose;

};

#endif
