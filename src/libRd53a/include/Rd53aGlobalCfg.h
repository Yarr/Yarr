#ifndef RD53AGLOBALCFG_H
#define RD53AGLOBALCFG_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A library
// # Comment: RD53A global register
// # Date: August 2017
// ################################

#include <iostream>
#include <string>
#include <map>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Rd53aReg {
    public:
        Rd53aReg() {}

        void init(uint16_t *cfg, const unsigned bOffset, const unsigned bits, const uint16_t value) {
            m_cfg = cfg;
            m_bOffset = bOffset;
            m_bits = bits;
            this->write(value);
        }

        void write(const uint16_t value) {
            unsigned mask = (1<<m_bits)-1;
            *m_cfg = (*m_cfg&(~(mask<<m_bOffset))) | ((value&mask)<<m_bOffset);
        }

        uint16_t read() const {
            unsigned mask = (1<<m_bits)-1;
            return ((*m_cfg >> m_bOffset) & mask);
        }
    protected:
    private:
        uint16_t *m_cfg;
        unsigned m_bOffset;
        unsigned m_bits;
};

class Rd53aGlobalCfg {
    public:
        static const unsigned numRegs = 137;
        Rd53aGlobalCfg();
        ~Rd53aGlobalCfg();
        void init();
    protected:
        void toFileJson(json &j);
        void fromFileJson(json &j);
    private:
        uint16_t m_cfg[numRegs];
    public:
        std::map<std::string, Rd53aReg*> regMap;

        //0
        Rd53aReg PixPortalHigh;
        Rd53aReg PixPortalLow;
        //1
        Rd53aReg RegionCol;
        //2
        Rd53aReg RegionRow;
        //3
        Rd53aReg PixMode;
        Rd53aReg BMask;
        //4
        Rd53aReg PixDefaultConfig;
        //5
        Rd53aReg Ibiasp1Sync;
        //6
        Rd53aReg Ibiasp2Sync;
        //7
        Rd53aReg IbiasSfSync;
        //8
        Rd53aReg IbiasKrumSync;
        //9
        Rd53aReg IbiasDiscSync;
        //10
        Rd53aReg IctrlSynctSync;
        //11
        Rd53aReg VblSync;
        //12
        Rd53aReg VthSync;
        //13
        Rd53aReg VrefKrumSync;
        //14
        Rd53aReg PaInBiasLin;
        //15
        Rd53aReg FcBiasLin;
        //16
        Rd53aReg KrumCurrLin;
        //17
        Rd53aReg LdacLin;
        //18
        Rd53aReg CompLin;
        //19
        Rd53aReg RefKrumLin;
        //20
        Rd53aReg VthresholdLin;
        //21
        Rd53aReg PrmpDiff;
        //22
        Rd53aReg FolDiff;
        //23
        Rd53aReg PrecompDiff;
        //24
        Rd53aReg CompDiff;
        //25
        Rd53aReg VffDiff;
        //26
        Rd53aReg Vth1Diff;
        //27
        Rd53aReg Vth2Diff;
        //28
        Rd53aReg LccDiff;
        //29
        Rd53aReg ConfFeDiff;
        //31
        Rd53aReg SldoAnalogTrim;
        Rd53aReg SldoDigitalTrim;
        //32
        Rd53aReg EnCoreColSync;
        //33
        Rd53aReg EnCoreColLin1;
        //34
        Rd53aReg EnCoreColLin2;
        //35
        Rd53aReg EnCoreColDiff1;
        //36
        Rd53aReg EnCoreColDiff2;
        //37
        Rd53aReg LatencyConfig;
        //38
        Rd53aReg WrSyncDelaySync;
        //39
        Rd53aReg InjModeDel; // split
        //40
        Rd53aReg ClkDataDelay; // split
        //41
        Rd53aReg VcalHigh;
        //42
        Rd53aReg VcalMed;
        //43
        Rd53aReg ChSyncConf; // split
        //44
        Rd53aReg GlobalPulseRt;
        //46
        Rd53aReg CalColprSync1;
        //47
        Rd53aReg CalColprSync2;
        //48
        Rd53aReg CalColprSync3;
        //49
        Rd53aReg CalColprSync4;
        //50
        Rd53aReg CalColprLin1;
        //51
        Rd53aReg CalColprLin2;
        //52
        Rd53aReg CalColprLin3;
        //53
        Rd53aReg CalColprLin4;
        //54
        Rd53aReg CalColprLin5;
        //55
        Rd53aReg CalColprDiff1;
        //56
        Rd53aReg CalColprDiff2;
        //57
        Rd53aReg CalColprDiff3;
        //58
        Rd53aReg CalColprDiff4;
        //59
        Rd53aReg CalColprDiff5;
        //60
        Rd53aReg DebugConfig;
        //61
        Rd53aReg OutputConfig; // split
        //62
        Rd53aReg OutPadConfig;
        //63
        Rd53aReg GpLvdsRoute;
        //64
        Rd53aReg CdrConfig;
        //65
        Rd53aReg VcoBuffBias;
        //66
        Rd53aReg CdrCpIbias;
        //67
        Rd53aReg VcoIbias;
        //68
        Rd53aReg SerSelOut;
        //69
        Rd53aReg CmlConfig; // TODO: this is composed of SER_INV_TAP, SER_EN_TAP, enable CMLs - how should this be broken up?
        //70
        Rd53aReg CmlTapBias0;
        //71
        Rd53aReg CmlTapBias1;
        //72
        Rd53aReg CmlTampBias2;
        //73
        Rd53aReg AuroraCcCfg; // split
        //74
        Rd53aReg AuroraCbCfg0; // split
        //75
        Rd53aReg AuroraCbCfg1;
        //76
        Rd53aReg AuroraInitWait;
        //77
        Rd53aReg MonitorMux; // split
        //78-97 TODO: what should the naming scheme be?
};
#endif

