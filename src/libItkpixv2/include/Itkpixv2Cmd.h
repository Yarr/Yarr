#ifndef ITKPIXV2CMD_H
#define ITKPIXV2CMD_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 Library
// # Comment: Collection of ITkPixV2 commands
// # Date: Jul 2023
// ################################

#include <array>

#include "TxCore.h"

class Itkpixv2Cmd {
    public:
        // custom 5b to 8b encoding
        constexpr static uint16_t enc5to8[32] = {
              0x6A, 0x6C, 0x71, 0x72,
              0x74, 0x8B, 0x8D, 0x8E,
              0x93, 0x95, 0x96, 0x99,
              0x9A, 0x9C, 0xA3, 0xA5,
              0xA6, 0xA9, 0x59, 0xAC, 
              0xB1, 0xB2, 0xB4, 0xC3, 
              0xC5, 0xC6, 0xC9, 0xCA, 
              0xCC, 0xD1, 0xD2, 0xD4  };
        constexpr static uint32_t encode5to8(uint32_t val) {
            return static_cast<uint32_t>( Itkpixv2Cmd::enc5to8[val & 0x1F] );
        }

        // Trigger Symbols
        constexpr static uint16_t encTrigger[16] = {
              0xAA, 0x2B, 0x2D, 0x2E,
              0x33, 0x35, 0x36, 0x39,
              0x3A, 0x3C, 0x4B, 0x4D,
              0x4E, 0x53, 0x55, 0x56  };

        // Symbols are reused for the Tags
        constexpr static uint16_t encTag[54] = {
              0x6A, 0x6C, 0x71, 0x72,
              0x74, 0x8B, 0x8D, 0x8E,
              0x93, 0x95, 0x96, 0x99,
              0x9A, 0x9C, 0xA3, 0xA5,
              0xA6, 0xA9, 0x59, 0xAC, 
              0xB1, 0xB2, 0xB4, 0xC3, 
              0xC5, 0xC6, 0xC9, 0xCA, 
              0xCC, 0xD1, 0xD2, 0xD4,
              0x63, 0x5A, 0x5C, 0xAA,
              0x65, 0x69, 0x2B, 0x2D,
              0x2E, 0x33, 0x35, 0x36, 
              0x39, 0x3A, 0x3C, 0x4B, 
              0x4D, 0x4E, 0x53, 0x55, 
              0x56, 0x66  };
    
        static std::array<uint16_t, 1> genPllLock();
        static std::array<uint16_t, 1> genSync();
        static std::array<uint16_t, 1> genTrigger(uint8_t bc, uint8_t tag);
        static std::array<uint16_t, 2> genReadTrigger(uint8_t chipId, uint8_t etag);
        static std::array<uint16_t, 1> genClear(uint8_t chipId);
        static std::array<uint16_t, 1> genGlobalPulse(uint8_t chipId);
        static std::array<uint16_t, 3> genCal(uint8_t chipId, uint8_t mode, uint8_t edgeDelay, uint8_t edgeDuration, uint8_t auxPar, uint8_t auxDelay);
        static std::array<uint16_t, 4> genWrReg(uint8_t chipId, uint16_t address, uint16_t data);
        static std::array<uint16_t, 2> genRdReg(uint8_t chipId, uint16_t address);

        void sendPllLock();
        void sendSync();
        void sendReadTrigger(uint8_t chipId, uint8_t etag);
        void sendClear(uint8_t chipId);
        void sendGlobalPulse(uint8_t chipId);
        void sendWrReg(uint8_t chipId, uint16_t address, uint16_t data);
        void sendRdReg(uint8_t chipId, uint16_t address);
        void sendPixRegBlock(uint8_t chipId, std::array<uint16_t, 384> &data);

    protected:
        Itkpixv2Cmd();
        Itkpixv2Cmd(TxCore *arg_core);
        ~Itkpixv2Cmd();

        void setCore(TxCore *arg_core) {
            core = arg_core;
        }
        
        TxCore *core;
    private:
        static uint16_t conv10Bit(uint16_t value);
};

#endif
