/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Utilities common to several classes
*/

#ifndef ITKPIXV2EMUUTILS_H
#define ITKPIXV2EMUUTILS_H

#include <cstdint>

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


}

#endif