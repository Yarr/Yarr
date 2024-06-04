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

    //Initialize the FE registers
    m_itkpixv2Cfg = std::make_unique<Itkpixv2Cfg>();

    //Initialize the command interpreter and exe
    m_cmdInterpreter = std::make_unique<Itkpixv2EmuCommandInterpreter>();
    m_cmdExe         = std::make_unique<Itkpixv2EmuCommandExe>();

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
    if (cmd.header == Itkpixv2EmuUtils::Commands::WrReg){
        std::bitset<32> d(cmd.data);
        rlog->info("WrReg command to address {} with data {}", cmd.address, cmd.data);
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