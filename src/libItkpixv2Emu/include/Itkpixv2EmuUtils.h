/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Utilities common to several classes
*/

#ifndef ITKPIXV2EMUUTILS_H
#define ITKPIXV2EMUUTILS_H

#include <cstdint>
#include <chrono>
#include <map>
#include <unordered_set>

namespace Itkpixv2EmuUtils {

    //Itkpixv2 commands in a human-readable form
    enum Commands : uint8_t{
        Sync        = 0b10000001,
        PLLlock     = 0b10101010,
        Clear       = 0b01011010,
        GlobalPulse = 0b01011100,
        Cal         = 0b01100011,
        WrReg       = 0b01100110,
        RdReg       = 0b01100101,
    };

    //Constant time interval
    static const std::chrono::nanoseconds m_ns10 = std::chrono::nanoseconds(10);

    //give the passed command/payload a human-readable interface
    struct Cmd {
        uint8_t header  = 0;
        union {
            uint8_t tag;
            uint8_t id ;
        };
        //The address here is 32 bit although only 9 will be used.
        //This way we don't need to overload the function filling these.
        uint32_t address = 0;
        uint32_t data    = 0;
    };

    //8to5 bit encoding lookup table. Using map, since the indices are not 1, 2, 3, ...
    static std::map<const uint8_t, const uint8_t> lut8to5 = {
        {0x6A, 0x00},
        {0x6C, 0x01},
        {0x71, 0x02},
        {0x72, 0x03},
        {0x74, 0x04},
        {0x8B, 0x05},
        {0x8D, 0x06},
        {0x8E, 0x07},
        {0x93, 0x08},
        {0x95, 0x09},
        {0x96, 0x0A},
        {0x99, 0x0B},
        {0x9A, 0x0C},
        {0x9C, 0x0D},
        {0xA3, 0x0E},
        {0xA5, 0x0F},
        {0xA6, 0x10},
        {0xA9, 0x11},
        {0x59, 0x12},
        {0xAC, 0x13}, 
        {0xB1, 0x14},
        {0xB2, 0x15},
        {0xB4, 0x16},
        {0xC3, 0x17}, 
        {0xC5, 0x18},
        {0xC6, 0x19},
        {0xC9, 0x1A},
        {0xCA, 0x1B}, 
        {0xCC, 0x1C},
        {0xD1, 0x1D},
        {0xD2, 0x1E},
        {0xD4, 0x1F}
    };

    //Trigger command collection
    static std::unordered_set<uint8_t> triggerCommands = {0x2B, 0x2D, 0x2E, 0x33, 0x35, 0x36, 0x39, 0x3A, 0x3C, 0x4B, 0x4D, 0x4E, 0x53, 0x55, 0x56};

    //Trigger patterns
    static std::map<const uint8_t, const uint8_t> lutTriggerPattern = {
        {0x2B, 0x1},
        {0x2D, 0x2},
        {0x2E, 0x3},
        {0x33, 0x4},
        {0x35, 0x5},
        {0x36, 0x6},
        {0x39, 0x7},
        {0x3A, 0x8},
        {0x3C, 0x9},
        {0x4B, 0xA},
        {0x4D, 0xB},
        {0x4E, 0xC},
        {0x53, 0xD},
        {0x55, 0xE},
        {0x56, 0xF}
    };



}

#endif