#ifndef __STAR_EMU_CFG__
#define __STAR_EMU_CFG__

#include <algorithm>

/*
  A temporary implementation of the Star chip register configuration as a mock-up of that defined in https://gitlab.cern.ch/YARR/YARR/blob/devel_FelixNetIO_StarChip/src/libStar/include/StarCfg.h
  Delete/Merge this after the two branches are ready to merged.
*/

namespace emu {

enum class HCCStarRegister
{
    SEU1=0, SEU2=1, SEU3=2, FrameRaw=3, LCBerr=4, ADCStatus=5, Status=6,
    HPR=15, Pulse=16, Addressing=17, 
    Delay1=32, Delay2=33, Delay3=34, PLL1=35, PLL2=36, PLL3=37,
    DRV1=38, DRV2=39, ICenable=40, OPmode=41, OPmodeC=42,
    Cfg1=43, Cfg2=44, ExtRst=45, ExtRstC=46, ErrCfg=47, ADCcfg=48
}; // HCCStarRegister
    
enum class ABCStarRegs
{
    // Special register: 0x00
    SCReg=0,
    // Analog and DCS registers: 0x01 - 0x0f
    ADCS1=1, ADCS2=2, ADCS3=3, ADCS4=4, ADCS5=5, ADCS6=6, ADCS7=7, 
    // Input/Mask registers: 0x10 - 0x17
    MaskInput0=16, MaskInput1=17, MaskInput2=18, MaskInput3=19,
    MaskInput4=20, MaskInput5=21, MaskInput6=22, MaskInput7=23,
    // Configuration registers: 0x20 - 0x2f
    CREG0=32, CREG1=33, CREG2=34, CREG3=35, CREG4=36, CREG5=37, CREG6=38,
    // Status registers: 0x30 - 0x3e
    STAT0=48, STAT1=49, STAT2=50, STAT3=51, STAT4=52,
    // High Priority Register: 0x3f
    HPR=63,
    // TrimDAC registers: 0x40 - 0x67
    TrimDAC0=64, TrimDAC1=65, TrimDAC2=66, TrimDAC3=67, TrimDAC4=68,
    TrimDAC5=69, TrimDAC6=70, TrimDAC7=71, TrimDAC8=72, TrimDAC9=73,
    TrimDAC10=74, TrimDAC11=75, TrimDAC12=76, TrimDAC13=77, TrimDAC14=78,
    TrimDAC15=79, TrimDAC16=80, TrimDAC17=81, TrimDAC18=82, TrimDAC19=83,
    TrimDAC20=84, TrimDAC21=85, TrimDAC22=86, TrimDAC23=87, TrimDAC24=88,
    TrimDAC25=89, TrimDAC26=90, TrimDAC27=91, TrimDAC28=92, TrimDAC29=93,
    TrimDAC30=94, TrimDAC31=95, TrimDAC32=96, TrimDAC33=97, TrimDAC34=98,
    TrimDAC35=99, TrimDAC36=100, TrimDAC37=101, TrimDAC38=102, TrimDAC39=103,
    // Calibration Enable registers: 0x68 - 0x6f
    CalREG0=104, CalREG1=105, CalREG2=106, CalREG3=107, CalREG4=108,
    CalREG5=109, CalREG6=110, CalREG7=111, 
    // Hit counters registers: 0x70 - 0xaf
    HitCountREG0=112, HitCountREG1=113, HitCountREG2=114, HitCountREG3=115,
    HitCountREG4=116, HitCountREG5=117, HitCountREG6=118, HitCountREG7=119,
    HitCountREG8=120, HitCountREG9=121, HitCountREG10=122, HitCountREG11=123,
    HitCountREG12=124, HitCountREG13=125, HitCountREG14=126, HitCountREG15=127,
    HitCountREG16=128, HitCountREG17=129, HitCountREG18=130, HitCountREG19=131,
    HitCountREG20=132, HitCountREG21=133, HitCountREG22=134, HitCountREG23=135,
    HitCountREG24=136, HitCountREG25=137, HitCountREG26=138, HitCountREG27=139,
    HitCountREG28=140, HitCountREG29=141, HitCountREG30=142, HitCountREG31=143,
    HitCountREG32=144, HitCountREG33=145, HitCountREG34=146, HitCountREG35=147,
    HitCountREG36=148, HitCountREG37=149, HitCountREG38=150, HitCountREG39=151,
    HitCountREG40=152, HitCountREG41=153, HitCountREG42=154, HitCountREG43=155,
    HitCountREG44=156, HitCountREG45=157, HitCountREG46=158, HitCountREG47=159,
    HitCountREG48=160, HitCountREG49=161, HitCountREG50=162, HitCountREG51=163,
    HitCountREG52=164, HitCountREG53=165, HitCountREG54=166, HitCountREG55=167,
    HitCountREG56=168, HitCountREG57=169, HitCountREG58=170, HitCountREG59=171,
    HitCountREG60=172, HitCountREG61=173, HitCountREG62=174, HitCountREG63=175
}; // ABCStarRegs

class StarCfg {
  public:
    StarCfg(){}
    ~StarCfg(){}

    void initRegisterMaps()
    {
        // load default register configurations
    }

    const uint32_t getHCCRegister(uint32_t addr)
    {
        if (registerMap[0].find(addr) == registerMap[0].end())
            return 0xdeadbeef; // for now
        else
            return registerMap[0][addr];
    }

    const uint32_t getHCCRegister(HCCStarRegister reg)
    {
        return getHCCRegister((uint32_t)reg);
    }

    void setHCCRegister(uint32_t addr, uint32_t val)
    {
        //if (registerMap[0].find(addr) != registerMap[0].end())
        registerMap[0][addr] = val;
    }

    void setHCCRegister(HCCStarRegister reg, uint32_t val)
    {
        setHCCRegister((uint32_t)reg, val);
    }

    const uint32_t getABCRegister(uint32_t addr, int chipID)
    {
        unsigned index = indexForABCchipID(chipID);
        if (index >  this->nABCs()) { // found no ABC chip with ID "chipID"
            std::cout << __PRETTY_FUNCTION__ << " : cannot find an ABCStar chip with ID = " << chipID << std::endl;
            return -1;
        }

        if (registerMap[index].find(addr) == registerMap[index].end())
            return 0xabadcafe; // for now
        else
            return registerMap[index][addr];
    }

    const uint32_t getABCRegister(ABCStarRegs reg, int chipID)
    {
        return getABCRegister((uint32_t)reg, chipID);
    }

    void setABCRegister(uint32_t addr, uint32_t val, int chipID)
    {
        unsigned index = indexForABCchipID(chipID);
        if (index > this->nABCs()) { // found no ABC chip with ID "chipID"
            std::cout << __PRETTY_FUNCTION__ << " : cannot find an ABCStar chip with ID = " << chipID << std::endl;
            return;
        }

        //if (registerMap[index].find(addr) != registerMap[index].end())
        registerMap[index][addr] = val;
    }

    void setABCRegister(ABCStarRegs reg, uint32_t val, int chipID)
    {
        setABCRegister((uint32_t)reg, val, chipID);
    }
    
    const uint32_t getABCSubRegValue(uint32_t addr, int chipID,
                                     uint8_t msb, uint8_t lsb)
    {
        assert(msb>=lsb);
        uint32_t regVal = this->getABCRegister(addr, chipID);
        return (regVal >> lsb) & ((1<<(msb-lsb+1))-1);
    }

    const uint32_t getABCSubRegValue(ABCStarRegs reg, int chipID,
                                     uint8_t msb, uint8_t lsb)
    {
        return getABCSubRegValue((uint32_t)reg, chipID, msb, lsb);
    }
    
    const int getHCCchipID(){return m_hccID;}
    void setHCCchipID(int hccID) {m_hccID = hccID;}

    const int getABCchipID(unsigned chipIndex)
    {
        assert(chipIndex); // chipIndex = 0 for HCC
        return m_ABCchipIDs[chipIndex-1];
    }
    void setABCchipIDs(const std::vector<int> abcIDs) {m_ABCchipIDs = abcIDs;}
    void addABC(int abcID) {m_ABCchipIDs.push_back(abcID);}
    
    const unsigned indexForABCchipID(int chipID)
    {
        return std::distance(m_ABCchipIDs.begin(), std::find(m_ABCchipIDs.begin(), m_ABCchipIDs.end(), chipID)) + 1;
    }

    unsigned nABCs() {return m_ABCchipIDs.size();}

    void init()
    {
        // Should get these e.g. from chip config json
        // for now
        this->setHCCchipID(10);
        this->setABCchipIDs({13,12,11,10,9,8,7,6,5,4,3});
        this->initRegisterMaps();
    }
    
  private:

    int m_hccID;
    std::vector<int> m_ABCchipIDs;

    // 2D map of HCCStar and ABCStars; chip_index: 0 for HCC, iABC+1 for ABC
    // registerMap[chip_index][addr]
    std::map<uint32_t, std::map<uint32_t, uint32_t> >registerMap;
    
}; // class StarCfg

} // namespace emu

#endif
