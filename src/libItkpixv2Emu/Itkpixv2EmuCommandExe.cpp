/*
* Author: Ondra Kovanda, ondrej.kovanda at cern.ch
* Date: 05/2024
* Description: Core functionality of the ITkPixV2 emulator
*/

#include "Itkpixv2EmuCommandExe.h"
//#include "logging.h"

//namespace {
//    auto rlog = logging::make_log("Itkpixv2EmuCommandExe");
//}


Itkpixv2EmuCommandExe::Itkpixv2EmuCommandExe(EmuCom* rx, std::shared_ptr<Itkpixv2Cfg>& cfg){

    //attach the output pipeline
    m_rx = rx;

    //link the registers
    m_cfg = cfg;

}

void Itkpixv2EmuCommandExe::exe(const Itkpixv2EmuUtils::Cmd cmd){

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
    
    //Can be either to pixel portal (register 0) or a global register
    switch (cmd.address){
        case 0 :
            //Registers PixRegionRow and PixRegionCol decide which of the pixel pairs the portal portals to
            m_cfg->pixRegs[m_cfg->PixRegionCol.read()][m_cfg->PixRegionRow.read()] = (cmd.data & 0xFFFF);
            //if AutoRow is enabled, increase the current row
            if (m_cfg->PixAutoRow.read()) m_cfg->PixRegionRow.write(m_cfg->PixRegionRow.read() + 1);
            break;

        default:
            (*m_cfg)[cmd.address] = (cmd.data & 0xFFFF);
            break;
    }

}

void Itkpixv2EmuCommandExe::doRdReg(const Itkpixv2EmuUtils::Cmd& cmd){

    //Do the simplest case for now - single chip, single RdReg command
    //Form the 64-bit block and fill it

    //Create an empty service block
    uint64_t serviceBlock = 0ULL;

    //add aurora identifier, chip ID and status
    uint64_t auroraKWord = 0x99;
    serviceBlock |= (auroraKWord << 56);

    uint64_t id = m_cfg->getChipId() & 0x3;
    serviceBlock |= (id << 54);

    uint64_t status = 0b0;
    serviceBlock |= (status << 52);

    //There are two cases what value will be read - either a pixel reg through pixel porta
    //or a global register. Different values of the address field apply in each case.
    uint64_t addressField = 0ULL;
    uint64_t value = 0ULL;
    switch (cmd.address){
        case 0:
            //if we read pixel register, the address is 1 + 9bit row address
            addressField = 0x200 | m_cfg->PixRegionRow.read();
            value |= m_cfg->pixRegs[m_cfg->PixRegionCol.read()][m_cfg->PixRegionRow.read()];
            //if AutoRow is enabled, increase the current row
            if (m_cfg->PixAutoRow.read()) m_cfg->PixRegionRow.write(m_cfg->PixRegionRow.read() + 1);

        default:
            //if we read global register, the address is just that
            addressField = cmd.address;
            value |= m_cfg->operator[](cmd.address);
    }

    //add 10 bits of address and 16 bits of register value
    serviceBlock |= (addressField << 42);
    serviceBlock |= (value << 26);

    m_rx->write32(serviceBlock >> 32);
    m_rx->write32(serviceBlock & 0xFFFFFFFF);
    
}