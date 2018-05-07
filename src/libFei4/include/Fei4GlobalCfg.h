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
#include <map>

#include "BitOps.h"
#include "Utils.h"
#include "tinyxml2.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Fei4Register {
    private:
        uint16_t *m_cfg;
        unsigned m_mOffset;
        unsigned m_bOffset;
        unsigned m_mask;
        bool m_msbRight;

    public:
        Fei4Register() {
            m_cfg = NULL;
            m_mOffset = 0;
            m_bOffset = 0;
            m_mask = 0;
            m_msbRight = false;
        };
        
        void initReg(uint16_t* cfg, const uint16_t &cfgBits, 
                const unsigned &mOffset, const unsigned &bOffset, 
                const unsigned &mask, const bool &msbRight=false) {
            m_cfg = cfg;
            m_mOffset = mOffset;
            m_bOffset = bOffset;
            m_mask = mask;
            m_msbRight = msbRight;
            this->write(cfgBits);
        }

        // Get value of field
        unsigned value() const{
            unsigned maskBits = (1<<m_mask)-1;
            unsigned tmp = ((m_cfg[m_mOffset]&(maskBits<<m_bOffset))>>m_bOffset);
            return (m_msbRight?BitOps::reverse_bits(tmp, m_mask):tmp);
        }

        // Write value to field and config
        void write(const uint16_t& cfgBits) {
            unsigned maskBits = (1<<m_mask)-1;
            m_cfg[m_mOffset]=(m_cfg[m_mOffset]&(~(maskBits<<m_bOffset))) | 
                (((m_msbRight?BitOps::reverse_bits(cfgBits, m_mask):cfgBits)&maskBits)<<m_bOffset);
        }

        unsigned addr() const{
            return m_mOffset;
        }

};


class Fei4GlobalCfg {
    private:
        void init();
    protected:
        void toFileJson(json &j);
        void fromFileJson(json &j);
    public:
        static const unsigned numRegs = 36;
        uint16_t cfg[numRegs];
        Fei4GlobalCfg();

        void toFilePlain(std::string filename);
        void fromFilePlain(std::string filename);

        void toFileXml(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *node);

        void setValue(Fei4Register Fei4GlobalCfg::*ref, const uint16_t& cfgBits) {
                (this->*ref).write(cfgBits);
            }

        uint16_t getValue(Fei4Register Fei4GlobalCfg::*ref) {
                return (this->*ref).value();
            }
        
        uint16_t getAddr(Fei4Register Fei4GlobalCfg::*ref) {
                return (this->*ref).addr();
            }

        uint16_t getValue(std::string regName) {
            if (regMap.find(regName) != regMap.end()) {
                return (this->*regMap[regName]).value();
            } else {
                std::cerr << " --> Error: Could not find register \""<< regName << "\"" << std::endl;
            }
            return 0;
        }

        void setValue(std::string regName, uint16_t value) {
            if (regMap.find(regName) != regMap.end()) {
                (this->*regMap[regName]).write(value);
            } else {
                std::cerr << " --> Error: Could not find register \""<< regName << "\"" << std::endl;
            }
        }

        std::map<std::string, Fei4Register Fei4GlobalCfg::*> regMap;

        // Fe-I4 global registers, see page 118 FE-I4B Manual
        //1
        Fei4Register SME;
        Fei4Register EventLimit;
        //2
        Fei4Register Trig_Count;
        Fei4Register Conf_AddrEnable;
        //3
        Fei4Register ErrorMask_0;
        //4
        Fei4Register ErrorMask_1;
        //5
        Fei4Register PrmpVbp_R;
        Fei4Register BufVgOpAmp;
        //Fei4Register GADCVref;
        //6
        Fei4Register PrmpVbp;
        //7
        Fei4Register TDACVbp;
        Fei4Register DisVbn;
        //8
        Fei4Register Amp2Vbn;
        Fei4Register Amp2VbpFol;
        //9
        Fei4Register Amp2Vbp;
        //10
        Fei4Register FDACVbn;
        Fei4Register Amp2Vbpff;
        //11
        Fei4Register PrmpVbnFol;
        Fei4Register PrmpVbp_L;
        //12
        Fei4Register PrmpVbpf;
        Fei4Register PrmpVbnLCC;
        //13
        Fei4Register S1;
        Fei4Register S0;
        Fei4Register Pixel_latch_strobe;
        //14
        Fei4Register LVDSDrvIref;
        Fei4Register GADCCompBias;
        //15
        Fei4Register PllIbias;
        Fei4Register LVDSDrvVos;
        //16
        Fei4Register TempSensIbias;
        Fei4Register PllIcp;
        //17
        Fei4Register PlsrIDACRamp;
        //18
        Fei4Register VrefDigTune;
        Fei4Register PlsrVgOpAmp;
        //19
        Fei4Register PlsrDACbias;
        Fei4Register VrefAnTune;
        //20
        Fei4Register Vthin_Coarse;
        Fei4Register Vthin_Fine;
        //21
        Fei4Register HitLD;
        Fei4Register DJO;
        Fei4Register DigHitIn_Sel;
        Fei4Register PlsrDAC;
        //22
        Fei4Register Colpr_Mode;
        Fei4Register Colpr_Addr;
        //23
        Fei4Register DisableColCnfg0;
        //24
        Fei4Register DisableColCnfg1;
        //25
        Fei4Register Trig_Lat;
        Fei4Register DisableColCnfg2;
        //26
        Fei4Register CMDcnt12;
        Fei4Register CalPulseWidth;
        Fei4Register CalPulseDelay;
        Fei4Register StopModeConfig;
        Fei4Register HitDiscCnfg;
        //27
        Fei4Register PLL_Enable;
        Fei4Register EFS;
        Fei4Register StopClkPulse;
        Fei4Register ReadErrorReq;
        Fei4Register GADC_En;
        Fei4Register SRRead;
        Fei4Register HitOr;
        Fei4Register CalEn;
        Fei4Register SRClr;
        Fei4Register Latch_Enable;
        Fei4Register SR_Clock;
        Fei4Register M13;
        //28
        Fei4Register LVDSDrvSet06;
        Fei4Register EN_40M;
        Fei4Register EN_80M;
        Fei4Register CLK1_S0;
        Fei4Register CLK1_S1;
        Fei4Register CLK1_S2;
        Fei4Register CLK0_S0;
        Fei4Register CLK0_S1;
        Fei4Register CLK0_S2;
        Fei4Register EN_160;
        Fei4Register EN_320;
        //29
        Fei4Register No8b10b;
        Fei4Register Clk2Out;
        Fei4Register EmptyRecordCnfg;
        Fei4Register LVDSDrvEn;
        Fei4Register LVDSDrvSet30;
        Fei4Register LVDSDrvSet12;
        //30
        Fei4Register TmpSensDiodeSel;
        Fei4Register TmpSensDisable;
        Fei4Register IleakRange;
        //31
        Fei4Register PlsrRiseUpTau;
        Fei4Register PlsrPwr;
        Fei4Register PlsrDelay;
        Fei4Register ExtDigCalSW;
        Fei4Register ExtAnaCalSW;
        Fei4Register GADCSel;
        //32
        Fei4Register SELB0;
        //33
        Fei4Register SELB1;
        //34
        Fei4Register SELB2;
        Fei4Register PrmpVbpMsbEn;
        //35
        Fei4Register EFUSE;
};

#endif
