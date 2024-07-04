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
Itkpixv2Emu::Itkpixv2Emu(EmuCom* tx, EmuCom* rx, int seed): m_tx(tx), m_rx(rx) {

    //Switch on    
    run  = true;

    //Initialize pixels
    initPixels(seed);

    //Initialize the FE registers. This is needed to be accessible both here for setting up the pixels
    //and in the command exe
    m_itkpixv2Cfg = std::make_shared<Itkpixv2Cfg>();

    //Initialize the command interpreter and exe
    m_cmdInterpreter = std::make_unique<Itkpixv2EmuCommandInterpreter>();
    m_cmdExe         = std::make_unique<Itkpixv2EmuCommandExe>(m_rx, m_itkpixv2Cfg);

}

void Itkpixv2Emu::executeLoop(){
    //This loop should only run if the chip is turned on
    if (!run) return;
    //Check for commands in tx. This check has to stay here because
    //of the overall run flag. Can think of moving that flag to the
    //interpreter class somehow...
    if (m_tx->isEmpty()){

        //If none, wait a bit and repeat the loop
        std::this_thread::sleep_for(Itkpixv2EmuUtils::m_ns10);
        executeLoop();
    }

    //Once the commands arrive, the interpreter should kick in
    Itkpixv2EmuUtils::Cmd cmd = m_cmdInterpreter->readCommand(m_tx);
    
    //debug
    
    switch (cmd.header){
        case Itkpixv2EmuUtils::Commands::Sync          :{
            rlog->info("Sync command with tag {}", cmd.tag);
            break;
        }
        case Itkpixv2EmuUtils::Commands::PLLlock       :{
            rlog->info("PLLlock command with tag {}", cmd.tag);
            break;
        }
        case Itkpixv2EmuUtils::Commands::Clear         :{
            rlog->info("Clear command with id {}", cmd.id);
            break;
        }
        case Itkpixv2EmuUtils::Commands::GlobalPulse   :{
            rlog->info("GlobalPulse command with id {}", cmd.id);
            break;
        }
        case Itkpixv2EmuUtils::Commands::Cal           :{
            rlog->info("Cal command with id {} with data {}", cmd.id, cmd.data);
            break;
        }
        case Itkpixv2EmuUtils::Commands::WrReg                   :{
            rlog->info("WrReg command to address {} with data {}", cmd.address, cmd.data);
            std::cout << &(*m_cmdExe.get()) << "\n";
            m_cmdExe->exe(cmd);
            break;
        }
        case Itkpixv2EmuUtils::Commands::RdReg                   :{
            rlog->info("RdReg command to address {} with data {}", cmd.address, cmd.data);
            break;
        }
        default : {
            if (Itkpixv2EmuUtils::triggerCommands.find(cmd.header) != Itkpixv2EmuUtils::triggerCommands.end()) rlog->info("Trigger command {} with counter {}", cmd.header, cmd.id);
            else rlog->info("Unknown command with header {} and tag {}", cmd.header, cmd.tag);
            break;
        }
        


    }

    executeLoop();
}

void Itkpixv2Emu::outputLoop(){

}

void Itkpixv2Emu::initPixels(int seed){

    //We need to initialize all pixels with slightly
    //Randomized threshold to reflect real chip behaviour
    //The PixelLayout called m_thresholds will hold
    //a deviation from 1, where 1 would be exactly the desired
    //set threshold. For the time being, setting the deviation
    //to 5 %.
    
    std::mt19937 gen(seed);
    std::normal_distribution gauss(1., 0.05);

    for (uint col = 0; col < 400; col++){
        for (uint row = 0; row < 384; row++){
            m_thresholds(col, row) = gauss(gen);
        }
    }

}

Itkpixv2Emu::~Itkpixv2Emu() = default;