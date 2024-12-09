/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#include "Itkpixv2Emu.h"
#include <iostream>
#include <bitset>

namespace {
    auto rlog = logging::make_log("emu_itkpixv2");
}


//The constructor needs to attach the virtual tx and rx cables
Itkpixv2Emu::Itkpixv2Emu(EmuCom* tx, EmuCom* rx, int id, int seed): m_tx(tx), m_rx(rx) {

    //Switch on    
    run  = true;

    //Initialize the FE registers. This is needed to be accessible both here for setting up the pixels
    //and in the command exe
    m_itkpixv2Cfg = std::make_shared<Itkpixv2Cfg>();
    m_itkpixv2Cfg->setChipId(id);

    //Initialize the command interpreter and exe
    m_cmdInterpreter = std::make_unique<Itkpixv2EmuCommandInterpreter>();
    m_commandBuffer  = m_cmdInterpreter->getBuffer();
    m_cmdExe         = std::make_unique<Itkpixv2EmuCommandExe>(m_rx, m_itkpixv2Cfg);

    //Initialize pixels
    m_cmdExe->initPixels(seed);

}

void Itkpixv2Emu::executeLoop(){
    //This loop should only run if the chip is turned on
    //if (!run) return;
    while (run) {
        //The following will be split into two threads in the future.

        //Check for commands in tx. This check has to stay here because
        //of the overall run flag. Can think of moving that flag to the
        //interpreter class somehow...
        if (m_tx->isEmpty()){

            //If none, wait a bit and repeat the loop
            std::this_thread::sleep_for(Itkpixv2EmuUtils::m_ns10);
            //executeLoop();
            continue;
        }

        //Once the commands arrive, the interpreter should kick in
        m_cmdInterpreter->readCommand(m_tx);

        //debug

        while (m_commandBuffer->size()){

            Itkpixv2EmuUtils::Cmd cmd = m_commandBuffer->front();

            switch (cmd.header){
                case Itkpixv2EmuUtils::Commands::Sync          :{
                    //rlog->info("Sync command with tag {}", cmd.tag);
                    break;
                }
                case Itkpixv2EmuUtils::Commands::PLLlock       :{
                    //rlog->info("PLLlock command with tag {}", cmd.tag);
                    break;
                }
                case Itkpixv2EmuUtils::Commands::Clear         :{
                    rlog->warn("Clear command received - potential reset not implemented!");
                    break;
                }
                case Itkpixv2EmuUtils::Commands::GlobalPulse   :{
                    rlog->warn("GlobalPulse command received - potential reset not implemented!");
                    break;
                }
                case Itkpixv2EmuUtils::Commands::Cal           :{
                    rlog->trace("Cal command with id {} with data {}", cmd.id, cmd.data);
                    m_cmdExe->exe(cmd);
                    break;
                }
                case Itkpixv2EmuUtils::Commands::WrReg                   :{
                    //rlog->trace("WrReg command to address 0x{:x} with data 0x{:x}", cmd.address, cmd.data);
                    m_cmdExe->exe(cmd);
                    break;
                }
                case Itkpixv2EmuUtils::Commands::RdReg                   :{
                    rlog->trace("RdReg command to address {} with data {}", cmd.address, cmd.data);
                    m_cmdExe->exe(cmd);
                    break;
                }
                default : {
                    if (Itkpixv2EmuUtils::triggerCommands.find(cmd.header) != Itkpixv2EmuUtils::triggerCommands.end()){
                        rlog->trace("Trigger command {} (pattern 0x{:x}), tag 0x{:x}, ", cmd.header, Itkpixv2EmuUtils::lutTriggerPattern[cmd.header], cmd.id);
                        m_cmdExe->exe(cmd);
                    }
                    //else rlog->info("Unknown command with header {} and tag {}", cmd.header, cmd.id);
                    break;
                }

            }
            m_commandBuffer->pop();
        }

    }
}

void Itkpixv2Emu::outputLoop(){

}

Itkpixv2Emu::~Itkpixv2Emu() = default;