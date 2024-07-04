/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Command interpreting functionality of the ITkPixV2 emulator, factorized from the main emulator class
*/

#include "Itkpixv2EmuCommandInterpreter.h"
#include <bitset>
#include <iostream>


namespace {
    auto rlog = logging::make_log("Itkpixv2EmuCommandInterpreter");
}

Itkpixv2EmuCommandInterpreter::Itkpixv2EmuCommandInterpreter(){
    
}

void Itkpixv2EmuCommandInterpreter::bufferCommandUnits(EmuCom* tx){
    //Read the 32-bit word from tx
    const uint32_t commandWord = tx->read32();

    //Break it into 4 8-bit units and store them in the buffer
    for (uint unitCount = 0; unitCount < 4; unitCount++) m_commandUnitBuffer.push((commandWord >> (3 - unitCount) * 8) & 0xFF);
}

void Itkpixv2EmuCommandInterpreter::addDecodedUnits(uint8_t unitCount, uint32_t &var){
    for (uint unit = 0; unit < unitCount; unit++){
        uint32_t decodedUnit = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
        m_commandUnitBuffer.pop();
        var |= (decodedUnit << (unitCount - unit - 1) * 5);
    }
}

Itkpixv2EmuUtils::Cmd Itkpixv2EmuCommandInterpreter::readCommand(EmuCom* tx){
    //Third attempt on a decent implementation. Previous attempts
    //avoiding the intermediate buffer were extremely complicated.

    //Buffer the commands if there are any in tx, and if there's
    //crittically low number of units in the buffer
    if (m_commandUnitBuffer.size() < m_commandUnitBufferMinSize){
        while (!tx->isEmpty() && m_commandUnitBuffer.size() < m_commandUnitBufferMaxSize){
            bufferCommandUnits(tx);
        }
    }

    //Get the return Cmd structure ready
    Itkpixv2EmuUtils::Cmd cmd;

    //If the buffer is empty, return an empty cmd
    if (m_commandUnitBuffer.empty()) return cmd;

    cmd.header = m_commandUnitBuffer.front();
    m_commandUnitBuffer.pop();

    //Now decide what command are we dealing with
    switch (cmd.header){
        case Itkpixv2EmuUtils::Commands::Sync          :{
            cmd.tag = m_commandUnitBuffer.front();
            m_commandUnitBuffer.pop();
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::PLLlock       :{
            cmd.tag = m_commandUnitBuffer.front();
            m_commandUnitBuffer.pop();
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::Clear         :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::GlobalPulse   :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::Cal           :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();
            
            //Besides the ID, we need 4 blocks of data (8-5 encoded)
            cmd.data = 0;
            addDecodedUnits(4, cmd.data);
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::WrReg                   :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();

            //Here we need 2 blocks of address and 4 blocks of data
            addDecodedUnits(2, cmd.address);
            addDecodedUnits(4, cmd.data);
            //The data are padded with four 0s, need to shift by those
            cmd.data >>= 4;
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::RdReg                   :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();

            //Here we need 2 blocks of address
            addDecodedUnits(2, cmd.address);
            return cmd;
        }

        default : {
            //Even the unknown command will have the second unit
            if (Itkpixv2EmuUtils::triggerCommands.find(cmd.header) != Itkpixv2EmuUtils::triggerCommands.end()) rlog->info("It's a trigger!!!");

            cmd.id = m_commandUnitBuffer.front();
            m_commandUnitBuffer.pop();
            return cmd;
        }
        


    }

}