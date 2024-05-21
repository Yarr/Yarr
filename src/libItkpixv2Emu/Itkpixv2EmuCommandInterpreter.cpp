/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Command interpreting functionality of the ITkPixV2 emulator, factorized from the main emulator class
*/

#include "Itkpixv2EmuCommandInterpreter.h"

Itkpixv2EmuCommandInterpreter::Itkpixv2EmuCommandInterpreter(){
    
    m_overflow = 0;
}

Itkpixv2EmuUtils::Cmd Itkpixv2EmuCommandInterpreter::readCommand(EmuCom* tx){
    //this is currently quite horrible, think of something better


    //This function needs to deal with several cases due to the fact
    //that commands are in 16-bit blocks, but the pipeline transfers them
    //in 32-bit words.
    bool emptyOverflow = false;

    //Return object
    Itkpixv2EmuUtils::Cmd cmd;

    //First command frame
    uint16_t cmdFrame;

    //Payload
    uint64_t cmdPayload = 0;

    //Is there anything in the leftover?
    if (m_overflow) cmdFrame = m_overflow;
    //If not, read in next 32 bits
    else {
        uint32_t cmdWord = tx->read32();
        cmdFrame   = cmdWord >> 16;
        m_overflow = cmdWord & 0xFFFF;
        emptyOverflow = true;
    }

    cmd.header = cmdFrame;

    //Decide what to do
    switch (cmdFrame >> 8){
        case Itkpixv2EmuUtils::Commands::Sync        : return cmd;
        case Itkpixv2EmuUtils::Commands::PLLlock     : return cmd;
        case Itkpixv2EmuUtils::Commands::Clear       : return cmd;
        case Itkpixv2EmuUtils::Commands::GlobalPulse : return cmd;
        case Itkpixv2EmuUtils::Commands::Cal         :
            if (emptyOverflow){
                //the current overflow is the first 16 bits of payload, and
                //we need another 16 bits
                cmdPayload &= (m_overflow << 48);
                uint64_t cmdWord = tx->read32();
            }            

    }

    



}