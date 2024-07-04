/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#include "Itkpixv2EmuCommandExe.h"

Itkpixv2EmuCommandExe::Itkpixv2EmuCommandExe(EmuCom* rx, std::shared_ptr<Itkpixv2Cfg>& cfg){

    //attach the output pipeline
    m_rx = rx;

    //link the registers
    m_cfg = cfg;

}

void Itkpixv2EmuCommandExe::exe(const Itkpixv2EmuUtils::Cmd& cmd){

    (this->*commandMap[cmd.header])(cmd);

}

void Itkpixv2EmuCommandExe::doSync(const Itkpixv2EmuUtils::Cmd& cmd){

}

void Itkpixv2EmuCommandExe::doPLLlock(const Itkpixv2EmuUtils::Cmd& cmd){

}

void Itkpixv2EmuCommandExe::doClear(const Itkpixv2EmuUtils::Cmd& cmd){

}

void Itkpixv2EmuCommandExe::doGlobalPulse(const Itkpixv2EmuUtils::Cmd& cmd){

}

void Itkpixv2EmuCommandExe::doCal(const Itkpixv2EmuUtils::Cmd& cmd){

}

void Itkpixv2EmuCommandExe::doWrReg(const Itkpixv2EmuUtils::Cmd& cmd){

}

void Itkpixv2EmuCommandExe::doRdReg(const Itkpixv2EmuUtils::Cmd& cmd){
    m_rx->write32(1);
}