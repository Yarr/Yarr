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
        constexpr static uint16_t enc5to8[32] = {
              0x6A, 0x6C, 0x71, 0x72,
              0x74, 0x8B, 0x8D, 0x8E,
              0x93, 0x95, 0x96, 0x99,
              0x9A, 0x9C, 0xA3, 0xA5,
              0xA6, 0xA9, 0xAA, 0xAC, 
              0xB1, 0xB2, 0xB4, 0xC3, 
              0xC5, 0xC6, 0xC9, 0xCA, 
              0xCC, 0xD1, 0xD2, 0xD4  };
    
        constexpr static uint16_t encTrigger[16] = {
              0x00, 0x2B, 0x2D, 0x2E,
              0x33, 0x35, 0x36, 0x39,
              0x3A, 0x3C, 0x4B, 0x4D,
              0x4E, 0x53, 0x55, 0x56  };
    
        // custom 5b to 8b encoding
        constexpr static uint32_t encode5to8(uint32_t val) {
            return static_cast<uint32_t>( Rd53aCmd::enc5to8[val & 0x1F] );
        }

        // Slow Commands
        void globalPulse(uint32_t chipId, uint32_t duration);
        void cal(uint32_t chipId, uint32_t mode, uint32_t delay, uint32_t duration, uint32_t aux_mode=0, uint32_t aux_delay=0);
        void wrRegister(uint32_t chipId, uint32_t address, uint16_t value);
        void wrRegisterBlock(uint32_t chipId, uint32_t address, uint16_t values[6]);
        void rdRegister(uint32_t chipId, uint32_t address);

        // Fast Commands
        void trigger(uint32_t bc, uint32_t tag, uint32_t bc2=0, uint32_t tag2=0);
        void ecr();
        void bcr();
        void sync();
        void idle();

        static uint32_t genTrigger(uint32_t bc, uint32_t tag, uint32_t bc2=0, uint32_t tag2=0);
        static uint32_t genCal(uint32_t chipId, uint32_t mode, uint32_t delay, uint32_t duration, uint32_t aux_mode, uint32_t aux_delay);
    protected:
        Rd53aCmd();
        Rd53aCmd(TxCore *arg_core);
        ~Rd53aCmd();

        void setCore(TxCore *arg_core) {
            core = arg_core;
        }

        TxCore *core;
    private:
        bool verbose;
};

#endif

