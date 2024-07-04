/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Command interpreting functionality of the ITkPixV2 emulator, factorized from the main emulator class
*/

#ifndef ITKPIXV2EMUCOMMANDINTERPRETER_H
#define ITKPIXV2EMUCOMMANDINTERPRETER_H

#include "Itkpixv2EmuUtils.h"
#include "logging.h"
#include "EmuCom.h"
#include <thread>
#include <queue>

class Itkpixv2EmuCommandInterpreter {

    //Nomenclature: command unit is 8 bits long
    //              command frame is the 16-bit building block of a command
    //              command word is the 32-bit word that arrives through tx
    //              command header is the first 8 bits
    //              command tag/FE ID is the second 8-bit of the first command frame
    //              payload is what follows the command block in long commands
    
    public:
        Itkpixv2EmuCommandInterpreter();

        //Read the next command, and return a (header, payload)
        //pair, which is then digested by the command exe
        Itkpixv2EmuUtils::Cmd readCommand(EmuCom* tx);


    private:
        //8-bit buffer for the command blocks. All attempts to avoid the
        //additional buffer structure led to hugely complicated sorting
        //of the command streams.
        const uint8_t m_commandUnitBufferMaxSize = 8;
        const uint8_t m_commandUnitBufferMinSize = 8;
        std::queue<uint8_t> m_commandUnitBuffer;
        void bufferCommandUnits(EmuCom* tx);

        //Decode several units into a certain variable
        void addDecodedUnits(uint8_t unitCount, uint32_t &var);
};


#endif