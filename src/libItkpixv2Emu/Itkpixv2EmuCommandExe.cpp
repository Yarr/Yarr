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
    
    //Can be either to pixel portal (register 0) or a global register.
    //Technically, only 9 bits represent the address, the 10-th bit
    //keeps track of the multiple-write mode
    switch (cmd.address & 0x1FF){
        case 0 :
            //The pixel register has the following structure:
            //left pixel (16 bits): [TDAC sign, TDAC (4 bits), HitBus, Injection Enable, Enable] + right pixel analogously
            //different treatment of single and multiple write,
            //distinguished by the 10-th bit (0x200)
            switch (cmd.address & 0x200){
                //if 0, perform a single-write style of all 16 bits
                case 0:
                    //Registers PixRegionRow and PixRegionCol decide which of the pixel pairs the portal portals to
                    m_cfg->pixRegs[m_cfg->PixRegionCol.read()][m_cfg->PixRegionRow.read()] = (cmd.data & 0xFFFF);
                    break;
                //if it's 1, perform multiple-write style of either
                //TDAC or enable bits
                case 1:
                    //Depending on the value of the writing mode register,
                    //write as requested
                    uint16_t& val = m_cfg->pixRegs[m_cfg->PixRegionCol.read()][m_cfg->PixRegionRow.read()];
                    
                    switch (m_cfg->PixConfMode.read()){
                        case 0:
                            //Write mask information, it comes in the 10 bits of payload as (p. 46 of the RD53C manual)
                            //last 10 bits of cmd.data = unused[9:8], right-pixel-mask[7:5], unused[4:3], left-pixel-mask[2:0]
                            //left pixel:
                            val = (val & 0xF8FF) | ((cmd.data << 4) & 0x0700);
                            //right pixel:
                            val = (val & 0xFFF8) | (cmd.data & 0x0007);
                            break;
                        case 1:
                            //Write TDAC information
                            //last 10 bits of cmd.data = right-pixel-TDAC[9:5], left-pixel-TDAC[4:0]
                            //left pixel:
                            val = (val & 0x07FF) | ((cmd.data << 6) & 0xF800);
                            //right pixel:
                            val = (val & 0xFF07) | ((cmd.data << 3) & 0x00F8);
                            break;
                    }
                    break;
            }
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

    //There are two cases what value will be read - either a pixel reg through pixel portal
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