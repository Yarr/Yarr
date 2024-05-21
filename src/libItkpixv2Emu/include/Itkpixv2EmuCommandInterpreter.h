/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Command interpreting functionality of the ITkPixV2 emulator, factorized from the main emulator class
*/

#ifndef ITKPIXV2EMUCOMMANDINTERPRETER_H
#define ITKPIXV2EMUCOMMANDINTERPRETER_H

#include "Itkpixv2EmuUtils.h"
#include "EmuCom.h"
#include <utility>
#include <thread>

class Itkpixv2EmuCommandInterpreter {
    
    public:
        Itkpixv2EmuCommandInterpreter();

        //Read the next command, and return a (header, payload)
        //pair, which is then digested by the command exe
        Itkpixv2EmuUtils::Cmd readCommand(EmuCom* tx);


    private:
        //The potential leftover 16-bits after reading first half
        //of the 32-bit word
        uint16_t m_overflow = 0;

};


#endif