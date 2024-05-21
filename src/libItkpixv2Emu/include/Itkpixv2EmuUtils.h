/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Utilities common to several classes
*/

#ifndef ITKPIXV2EMUUTILS_H
#define ITKPIXV2EMUUTILS_H

#include <cstdint>
#include <chrono>

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
        uint16_t header  = 0;
        uint64_t payload = 0;
    };



}

#endif