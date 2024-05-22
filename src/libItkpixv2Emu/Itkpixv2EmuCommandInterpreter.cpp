/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Command interpreting functionality of the ITkPixV2 emulator, factorized from the main emulator class
*/

#include "Itkpixv2EmuCommandInterpreter.h"
#include <bitset>


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
    //avoidign the intermediate buffer were extremely complicated.

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
        case Itkpixv2EmuUtils::WrReg                   :{
            cmd.id  = Itkpixv2EmuUtils::lut8to5[m_commandUnitBuffer.front()];
            m_commandUnitBuffer.pop();

            //Here we need 2 blocks of address and 4 blocks of data
            addDecodedUnits(2, cmd.address);
            addDecodedUnits(4, cmd.data);
            //The data are padded with four 0s, need to shift by those
            cmd.data >>= 4;
            return cmd;
        }
        case Itkpixv2EmuUtils::RdReg                   :{
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


    /*   
    
    //This is really horrible and should be made more elegant.
    //An intermediate buffer for 16-bit frames would help, but
    //I wanted to avoid another storage.


    //This function needs to deal with several cases due to the fact
    //that commands are in 16-bit blocks, but the pipeline transfers them
    //in 32-bit words.

    //Make sure the command buffer is empty and the frame counter reset
    m_commandBuffer = 0;
    m_frameCounter = 0;

    //If there's anything in the overflow, put it in the command buffer
    if (m_overflow) {
        m_commandBuffer = (m_commandBuffer | m_overflow) << 48;
        m_frameCounter++;
    }
    //Else, read in the full 32-bit word (2 frames)
    else {
        m_commandBuffer |= tx->read32();
        m_frameCounter += 2;
        m_commandBuffer <<= 32;
    }
    
    //At this point, we have a command header and either the first
    //payload frame or an overflow. That depends on the actual command
    //received. We can put the command header into the output struct.
    Itkpixv2EmuUtils::Cmd cmd;
    //cmd.header = m_commandBuffer >> 48;

    //The command symbol (first 8 bits) decide the command to be run

    switch (m_commandBuffer >> 56){
        //Sync, PLLlock, Clear and GlobalPulse are all 16-bit only.
        //In those cases, we only want the first frame, and the potential
        //second one needs to go to overflow.
        case Itkpixv2EmuUtils::Commands::Sync        :{
            rlog->info("Received Sync Command");
            m_overflow = (m_commandBuffer >> 32) & 0x0000FFFF;
            cmd.header = Itkpixv2EmuUtils::Commands::Sync;
            cmd.tag    = 0b01111110;
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::PLLlock     :{
            rlog->info("Received PLLlock Command");
            m_overflow = (m_commandBuffer >> 32) & 0x0000FFFF;
            cmd.header = Itkpixv2EmuUtils::Commands::PLLlock;
            cmd.tag    = 0b10101010;
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::Clear       :{
            rlog->info("Received Clear Command");
            m_overflow = (m_commandBuffer >> 32) & 0x0000FFFF;
            cmd.header = Itkpixv2EmuUtils::Commands::Clear;
            const uint8_t idTag = (m_commandBuffer >> 48) & 0x00FF;
            cmd.id     = Itkpixv2EmuUtils::lut8to5.find(idTag);
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::GlobalPulse :{
            rlog->info("Received GlobalPulse Command");
            m_overflow = (m_commandBuffer >> 32) & 0x0000FFFF;
            cmd.header = Itkpixv2EmuUtils::Commands::GlobalPulse;
            const uint8_t idTag = (m_commandBuffer >> 48) & 0x00FF;
            cmd.id     = Itkpixv2EmuUtils::lut8to5.find(idTag);
            return cmd;
        }
        
        //Cal, WrReg and RdReg have a payload
        case Itkpixv2EmuUtils::Commands::Cal         :{
            rlog->info("Received Cal Command");
            //Cal has 2 frames of payload. We always need to
            //read in, regardless of the overflow.
            uint64_t commandWord = 0;
            commandWord |= tx->read32();
            m_commandBuffer |= commandWord << (2 - m_frameCounter) * 16;

            cmd.header = Itkpixv2EmuUtils::Commands::GlobalPulse;
            const uint8_t idTag = (m_commandBuffer >> 48) & 0x00FF;
            cmd.id     = Itkpixv2EmuUtils::lut8to5.find(idTag);
            cmd.address  = 0;
            cmd.address |= 
            //Put second and third block to cmd, rest to overflow
            cmd.payload = (m_commandBuffer & 0x0000FFFFFFFF0000) << 16;
            m_overflow  = (m_commandBuffer & 0x000000000000FFFF);
            return cmd;
        }
        case Itkpixv2EmuUtils::Commands::WrReg       :{
            rlog->info("Received WrReg Command");
            //WrReg has either 3 or 2 frames of payload, depending
            //on the first payload bit

            //First, decide which of the two it will be
            if (m_frameCounter == 1){
                uint64_t commandWord = 0;
                commandWord |= tx->read32();
                m_commandBuffer |= commandWord << (2 - m_frameCounter) * 16;
                m_frameCounter += 2;
            }

            //The 17th bit now decides. Handle the 2-frame case first
            if (m_commandBuffer >> (64 - 17) & 0x1){
                //There may not be enough frames in the buffer
                if (m_frameCounter == 2) {
                    uint64_t commandWord = 0;
                    m_commandBuffer |= tx->read32();
                    m_frameCounter += 2;
                }
                cmd.payload = (m_commandBuffer & 0x0000FFFFFFFF0000) << 16;
                m_overflow  = (m_commandBuffer & 0x000000000000FFFF);
                return cmd;
            }

            //If we're not returning 2 frames, then we're returning 3 frames
            //of payload, i. e. the whole buffer. Need to make sure it's full.
            //At this point, the buffer has either 2 or 3 frames in it.
            else {
                if (m_frameCounter == 2){
                    m_commandBuffer |= tx->read32();
                }
                else {
                    uint32_t commandWord = tx->read32();
                    m_commandBuffer |= commandWord >> 16;
                    m_overflow = commandWord & 0x0000FFFF;
                }
                cmd.payload = m_commandBuffer << 16;
                return cmd;
            }
        }
        case Itkpixv2EmuUtils::Commands::RdReg       :{
            rlog->info("Received RdReg Command");
            //RdReg has one frame of payload. Right now we have one or two
            //frames in the buffer.
            if (m_frameCounter == 1){
                uint64_t commandWord = tx->read32();
                m_commandBuffer |= commandWord << 16;
            }
            cmd.payload = (m_commandBuffer & 0x0000FFFF00000000) << 16;
            m_overflow  = (m_commandBuffer & 0x00000000FFFF0000) >> 16;
            return cmd;
        }
        default :{
            rlog->info("Received Unknown Command");
            //Unknown symbol. Don't do anything aside putting the potential
            //Next block into the overflow.
            m_overflow = (m_commandBuffer & 0x0000FFFF00000000) >> 32;
            return cmd;
        }
    }

    */

}