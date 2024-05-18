/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#include "Itkpixv2Emu.h"
#include <iostream>

namespace {
    auto rlog = logging::make_log("emu_itkpixv2");
}


//The constructor needs to attach the virtual tx and rx cables
Itkpixv2Emu::Itkpixv2Emu(EmuCom* tx, EmuCom* rx, int seed): m_tx(tx), m_rx(rx) {

    //Switch on    
    run  = true;

    //Initialize pixels
    initPixels(seed);

}

void Itkpixv2Emu::executeLoop(){
    //This loop should only run if the chip is turned on
    if (!run) return;

    //Check for commands in tx
    if (m_tx->isEmpty()){
        //std::cout << "waiting\n";
        //if none, wait a bit and repeat the loop
        std::this_thread::sleep_for(m_ns10);
        executeLoop();
    }

    //Once the commands arrive, read them in
    readCommand();

    //rlog->info("Received command {}", m_commandStream.front());

}

void Itkpixv2Emu::readCommand(){

    //pop the 32-bit word that arrived
    uint32_t w = m_tx->read32();

    //split it into two 16-bit commands to be buffered
    m_commandStream.push_back((w >> 16) & 0x0000FFFF);
    m_commandStream.push_back(w & 0x0000FFFF);
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