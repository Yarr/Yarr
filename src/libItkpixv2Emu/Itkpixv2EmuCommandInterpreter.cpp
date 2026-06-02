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

std::queue<Itkpixv2EmuUtils::Cmd>* Itkpixv2EmuCommandInterpreter::getBuffer(){
    return &m_commandsOut;
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

void Itkpixv2EmuCommandInterpreter::readCommand(EmuCom* tx){
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
    if (m_commandUnitBuffer.empty()) return;

    cmd.header = m_commandUnitBuffer.front();
    m_commandUnitBuffer.pop();

    //Now decide what command are we dealing with
    switch (cmd.header){
        case Itkpixv2EmuUtils::Commands::Sync          :{
            cmd.id = m_commandUnitBuffer.front();
            m_commandUnitBuffer.pop();
            //return cmd;
            m_commandsOut.push(cmd);
            break;
        }
        case Itkpixv2EmuUtils::Commands::PLLlock       :{
            cmd.id = m_commandUnitBuffer.front();
            m_commandUnitBuffer.pop();
            //return cmd;
            m_commandsOut.push(cmd);
            break;
        }
        case Itkpixv2EmuUtils::Commands::Clear         :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();
            //return cmd;
            m_commandsOut.push(cmd);
            break;
        }
        case Itkpixv2EmuUtils::Commands::GlobalPulse   :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();
            //return cmd;
            m_commandsOut.push(cmd);
            break;
        }
        case Itkpixv2EmuUtils::Commands::Cal           :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();
            
            //Besides the ID, we need 4 blocks of data (8-5 encoded)
            cmd.data = 0;
            addDecodedUnits(4, cmd.data);
            //return cmd;
            m_commandsOut.push(cmd);
            break;
        }
        case Itkpixv2EmuUtils::Commands::WrReg                   :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();

            //the first bit of address decides between single and multiple write
            switch (m_commandUnitBuffer.front() >> 7){
                //if it's zero, produce single WrReg command
                case 0:{
                    //Here we need 2 blocks of address and 4 blocks of data
                    addDecodedUnits(2, cmd.address);
                    addDecodedUnits(4, cmd.data);
                    //The data are padded with four 0s, need to shift by those
                    cmd.data >>= 4;
                    //return cmd;
                    m_commandsOut.push(cmd);
                    break;
                }
                //if one, we need to produce multiple single-WrReg commands to fake the
                //multiple write
                case 1:{
                    //pop the two blocks of address, it's 0 anyway
                    //rlog->info("WrReg(1) command!!!");
                    m_commandUnitBuffer.pop();
                    m_commandUnitBuffer.pop();
                    //keep track of the multiple-write mode in the 10-th bit
                    cmd.address = 0x200;
                    //keep adding the payloads until we see another command header
                    while (true){
                        //ensure that there are enough pre-bufferred command units.
                        //For we'll always need at least 2 to be present. For the time being,
                        //assume that there is something in the tx.
                        if (m_commandUnitBuffer.size() < 2) bufferCommandUnits(tx);
                        if (m_commandUnitBuffer.front() == Itkpixv2EmuUtils::Commands::Sync       ) break;
                        if (m_commandUnitBuffer.front() == Itkpixv2EmuUtils::Commands::PLLlock    ) break;
                        if (m_commandUnitBuffer.front() == Itkpixv2EmuUtils::Commands::Clear      ) break;
                        if (m_commandUnitBuffer.front() == Itkpixv2EmuUtils::Commands::GlobalPulse) break;
                        if (m_commandUnitBuffer.front() == Itkpixv2EmuUtils::Commands::Cal        ) break;
                        if (m_commandUnitBuffer.front() == Itkpixv2EmuUtils::Commands::WrReg      ) break;
                        if (m_commandUnitBuffer.front() == Itkpixv2EmuUtils::Commands::RdReg      ) break;
                        addDecodedUnits(2, cmd.data);
                        m_commandsOut.push(Itkpixv2EmuUtils::Cmd(cmd));
                        cmd.data = 0x00000000;
                    }

                    break;
                }
                default:
                    break;
            }
            break;
        }
        case Itkpixv2EmuUtils::Commands::RdReg                   :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();

            //Here we need 2 blocks of address
            addDecodedUnits(2, cmd.address);
            //return cmd;
            m_commandsOut.push(cmd);
            break;
        }

        default : {
            //Even the unknown command will have the second unit
            //if (Itkpixv2EmuUtils::triggerCommands.find(cmd.header) != Itkpixv2EmuUtils::triggerCommands.end()) rlog->info("It's a trigger!!!");

            cmd.id = m_commandUnitBuffer.front();
            m_commandUnitBuffer.pop();
            //return cmd;
            m_commandsOut.push(cmd);
            break;
        }
        


    }

}