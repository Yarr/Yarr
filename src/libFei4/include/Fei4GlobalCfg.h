#ifndef FEI4GLOBALCFG_H
#define FEI4GLOBALCFG_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 cpp Library
// # Comment: FE-I4 Global Register container
// ################################

#include <iostream>
#include <stdint.h>
#include <string>

#include "BitOps.h"

template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight = false>
class Field {
    private:
        T* m_cfg;

    public:
        Field(){};
        
        void initField(T* cfg, const T& cfgBits) {
            m_cfg = cfg;
            this->write(cfgBits);
        }
        // Get value of field
        unsigned value() const{
            unsigned maskBits = (1<<mask)-1;
            return ((m_cfg[mOffset]&(maskBits<<bOffset))>>bOffset);
        }

        // Write value to field and config
        void write(const T& cfgBits) {
            unsigned maskBits = (1<<mask)-1;
            m_cfg[mOffset]=(m_cfg[mOffset]&(~(maskBits<<bOffset))) | 
                (((msbRight?BitOps::reverse_bits(cfgBits, mask):cfgBits)&maskBits)<<bOffset);
        }

        unsigned addr() const{
            return mOffset;
        }
};

class Fei4GlobalCfg {
    private:
        void init();
    protected:
    public:
        static const unsigned numRegs = 36;
        uint16_t cfg[numRegs];
        Fei4GlobalCfg();

        void toFile(std::string filename);
        void fromFile(std::string filename);

        template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
            void setValue(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref, const T& cfgBits) {
                (this->*ref).write(cfgBits);
            }

        template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
            uint16_t getValue(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref) {
                return (this->*ref).value();
            }
        
        template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
            uint16_t getAddr(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref) {
                return (this->*ref).addr();
            }


        // Fe-I4 global registers, see page 118 FE-I4B Manual
        //1
        Field<uint16_t, 1, 0x8, 1> SME;
        Field<uint16_t, 1, 0x0, 8, true> EventLimit;
        //2
        Field<uint16_t, 2, 0xC, 4> Trig_Count;
        Field<uint16_t, 2, 0xB, 1> Conf_AddrEnable;
        //3
        Field<uint16_t, 3, 0x0, 16> ErrorMask_0;
        //4
        Field<uint16_t, 4, 0x0, 16> ErrorMask_1;
        //5
        Field<uint16_t, 5, 0x8, 8, true> PrmpVbp_R;
        Field<uint16_t, 5, 0x0, 8,true> BufVgOpAmp;
        Field<uint16_t, 5, 0x0, 8,true> GADCVref;
        //6
        Field<uint16_t, 6, 0x0, 8, true> PrmpVbp;
        //7
        Field<uint16_t, 7, 0x8, 8, true> TDACVbp;
        Field<uint16_t, 7, 0x0, 8, true> DisVbn;
        //8
        Field<uint16_t, 8, 0x8, 8, true> Amp2Vbn;
        Field<uint16_t, 8, 0x0, 8, true> Amp2VbpFol;
        //9
        Field<uint16_t, 9, 0x0, 8, true> Amp2Vbp;
        //10
        Field<uint16_t, 10, 0x8, 8, true> FDACVbn;
        Field<uint16_t, 10, 0x0, 8, true> Amp2Vbpff;
        //11
        Field<uint16_t, 11, 0x8, 8, true> PrmpVbnFol;
        Field<uint16_t, 11, 0x0, 8, true> PrmpVbp_L;
        //12
        Field<uint16_t, 12, 0x8, 8, true> PrmpVbpf;
        Field<uint16_t, 12, 0x0, 8, true> PrmpVbnLCC;
        //13
        Field<uint16_t, 13, 0xF, 1> S1;
        Field<uint16_t, 13, 0xE, 1> S0;
        Field<uint16_t, 13, 0x1, 13, true> Pixel_latch_strobe;
        //14
        Field<uint16_t, 14, 0x8, 8, true> LVDSDrvIref;
        Field<uint16_t, 14, 0x0, 8, true> GADCCompBias;
        //15
        Field<uint16_t, 15, 0x8, 8, true> PllIbias;
        Field<uint16_t, 15, 0x0, 8, true> LVDSDrvVos;
        //16
        Field<uint16_t, 16, 0x8, 8, true> TempSensIbias;
        Field<uint16_t, 16, 0x0, 8, true> PllIcp;
        //17
        Field<uint16_t, 17, 0x0, 8, true> PlsrIDACRamp;
        //18
        Field<uint16_t, 18, 0x8, 8, true> VrefDigTune;
        Field<uint16_t, 18, 0x0, 8, true> PlsrVgOpAmp;
        //19
        Field<uint16_t, 19, 0x8, 8, true> PlsrDACbias;
        Field<uint16_t, 19, 0x0, 8, true> VrefAnTune;
        //20
        Field<uint16_t, 20, 0x8, 8,true> Vthin_Coarse;
        Field<uint16_t, 20, 0x0, 8, true> Vthin_Fine;
        //21
        Field<uint16_t, 21, 0xC, 1> HitLD;
        Field<uint16_t, 21, 0xB, 1> DJO;
        Field<uint16_t, 21, 0xA, 1> DigHitIn_Sel;
        Field<uint16_t, 21, 0x0, 10, true> PlsrDAC;
        //22
        Field<uint16_t, 22, 0x8, 2, true> Colpr_Mode;
        Field<uint16_t, 22, 0x2, 6, true> Colpr_Addr;
        //23
        Field<uint16_t, 23, 0x0, 16> DisableColCnfg0;
        //24
        Field<uint16_t, 24, 0x0, 16> DisableColCnfg1;
        //25
        Field<uint16_t, 25, 0x8, 8> Trig_Lat;
        Field<uint16_t, 25, 0x0, 8> DisableColCnfg2;
        //26
        Field<uint16_t, 26, 0x3, 13> CMDcnt12;
        Field<uint16_t, 26, 0x3, 8> CalPulseWidth;
        Field<uint16_t, 26, 0xB, 5> CalPulseDelay;
        Field<uint16_t, 26, 0x2, 1> StopModeConfig;
        Field<uint16_t, 26, 0x0, 2> HitDiscCnfg;
        //27
        Field<uint16_t, 27, 0xF, 1> PLL_Enable;
        Field<uint16_t, 27, 0xE, 1> EFS;
        Field<uint16_t, 27, 0xD, 1> StopClkPulse;
        Field<uint16_t, 27, 0xC, 1> ReadErrorReq;
        Field<uint16_t, 27, 0xA, 1> GADC_En;
        Field<uint16_t, 27, 0x9, 1> SRRead;
        Field<uint16_t, 27, 0x5, 1> HitOr;
        Field<uint16_t, 27, 0x4, 1> CalEn;
        Field<uint16_t, 27, 0x3, 1> SRClr;
        Field<uint16_t, 27, 0x2, 1> Latch_Enable;
        Field<uint16_t, 27, 0x1, 1> SR_Clock;
        Field<uint16_t, 27, 0x0, 1> M13;
        //28
        Field<uint16_t, 28, 0xF, 1> LVDSDrvSet06;
        Field<uint16_t, 28, 0x9, 1> EN_40M;
        Field<uint16_t, 28, 0x8, 1> EN_80M;
        Field<uint16_t, 28, 0x7, 1> CLK1_S0;
        Field<uint16_t, 28, 0x6, 1> CLK1_S1;
        Field<uint16_t, 28, 0x5, 1> CLK1_S2;
        Field<uint16_t, 28, 0x4, 1> CLK0_S0;
        Field<uint16_t, 28, 0x3, 1> CLK0_S1;
        Field<uint16_t, 28, 0x2, 1> CLK0_S2;
        Field<uint16_t, 28, 0x1, 1> EN_160;
        Field<uint16_t, 28, 0x0, 1> EN_320;
        //29
        Field<uint16_t, 29, 0xD, 1> No8b10b;
        Field<uint16_t, 29, 0xC, 1> Clk2Out;
        Field<uint16_t, 29, 0x4, 8> EmptyRecordCnfg;
        Field<uint16_t, 29, 0x2, 1> LVDSDrvEn;
        Field<uint16_t, 29, 0x1, 1> LVDSDrvSet30;
        Field<uint16_t, 29, 0x0, 1> LVDSDrvSet12;
        //30
        Field<uint16_t, 30, 0xE, 2> TmpSensDiodeSel;
        Field<uint16_t, 30, 0xD, 1> TmpSensDisable;
        Field<uint16_t, 30, 0xC, 1> IleakRange;
        //31
        Field<uint16_t, 31, 0xD, 3> PlsrRiseUpTau;
        Field<uint16_t, 31, 0xC, 1> PlsrPwr;
        Field<uint16_t, 31, 0x6, 6, true> PlsrDelay;
        Field<uint16_t, 31, 0x5, 1> ExtDigCalSW;
        Field<uint16_t, 31, 0x4, 1> ExtAnaCalSW;
        Field<uint16_t, 31, 0x0, 3> GADCSel;
        //32
        Field<uint16_t, 32, 0x0, 16> SELB0;
        //33
        Field<uint16_t, 33, 0x0, 16> SELB1;
        //34
        Field<uint16_t, 34, 0x8, 8> SELB2;
        Field<uint16_t, 34, 0x4, 1> PrmpVbpMsbEn;
        //35
        Field<uint16_t, 35, 0x0, 16> EFUSE;


};

#endif
