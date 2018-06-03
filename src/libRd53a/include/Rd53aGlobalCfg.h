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
#include <cmath>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Rd53aReg {
    public:
        Rd53aReg() {
            m_addr = 999;
        }

        void init(unsigned addr, uint16_t *cfg, const unsigned bOffset, const unsigned bits, const uint16_t value) {
            m_addr = addr;
            m_cfg = cfg;
            m_bOffset = bOffset;
            m_bits = bits;
            this->write(value);
        }

        virtual void write(const uint16_t value) {
            unsigned mask = (1<<m_bits)-1;
            *m_cfg = (*m_cfg&(~(mask<<m_bOffset))) | ((value&mask)<<m_bOffset);
        }

        virtual uint16_t read() const {
            unsigned mask = (1<<m_bits)-1;
            return ((*m_cfg >> m_bOffset) & mask);
        }

        unsigned addr() const{
            return m_addr;
        }

        unsigned bits() const {
            return m_bits;
        }

    protected:
        uint16_t *m_cfg;
        unsigned m_bOffset;
        unsigned m_bits;
        unsigned m_addr;
    private:
};

class Rd53aDiffReg : public Rd53aReg {
    public:
        void init(Rd53aReg *arg_lowRef, Rd53aReg *arg_highRef, bool changeHigh) {
            lowRef = arg_lowRef;
            highRef = arg_highRef;
            m_cfg = NULL; // Not needed
            m_bOffset = 0; // Not needed
            m_bits = 0; // Not needed
            m_changeHigh = changeHigh;
            if (m_changeHigh) {
                m_addr = highRef->addr(); // Write register asks for the address, only want to modify highRef
            } else {
                m_addr = lowRef->addr();
            }

        }

        void write(const uint16_t value) override {
            uint16_t highValue = highRef->read();
            uint16_t lowValue = lowRef->read();
            //std::cout << __PRETTY_FUNCTION__ << " : " << value << " " << highValue << " " << lowValue <<  std::endl;
            if (m_changeHigh) {
                if (lowValue + value < pow(2, highRef->bits())) {
                    highRef->write(value + lowValue);
                } else {
                    std::cerr << "#ERROR# Could not write value to Rd53aDiffReg! Out of range!" << std::endl;
                }
            } else {
                if (highValue - value >= 0) {
                    lowRef->write(highValue - value);
                } else {
                    std::cerr << "#ERROR# Could not write value to Rd53aDiffReg! Out of range!" << std::endl;
                }
            }

        }

        uint16_t read() const override {
            uint16_t lowValue = lowRef->read();
            uint16_t highValue = highRef->read();
            return highValue - lowValue;
        }
    private:
        Rd53aReg *lowRef;
        Rd53aReg *highRef;
        bool m_changeHigh;
};

class Rd53aGlobalCfg {
    public:
        static constexpr unsigned numRegs = 138;
        std::array<uint16_t, numRegs> m_cfg;
        
        Rd53aGlobalCfg();
        ~Rd53aGlobalCfg();
        void init();

        uint16_t getValue(Rd53aReg Rd53aGlobalCfg::*ref) {
            return (this->*ref).read();
        }

        uint16_t getValue(std::string name) {
            if (regMap.find(name) != regMap.end()) {
                return (this->*regMap[name]).read();
            } else {
                std::cerr << " --> Error: Could not find register \"" << name << "\"" << std::endl;
            }
            return 0;
        }

        void setValue(Rd53aReg Rd53aGlobalCfg::*ref, uint16_t val) {
            (this->*ref).write(val);
        }

        void setValue(std::string name, uint16_t val) {
            if (regMap.find(name) != regMap.end()) {
                (this->*regMap[name]).write(val);
            } else {
                std::cerr << " --> Error: Could not find register \"" << name << "\"" << std::endl;
            }
        }

    std::string regName(const unsigned addr) const {
        std::string name = "null_register";

        for( auto& pair : regMap ) {
            if( addr == (this->*pair.second).addr() ) {
                name = pair.first;
                break;
            }
        }
        return name;
    }

    protected:
        void toFileJson(json &j);
        void fromFileJson(json &j);
    private:
    public:
        std::map<std::string, Rd53aReg Rd53aGlobalCfg::*> regMap;

        //Special diff registers
        Rd53aDiffReg InjVcalDiff;

        //0
        Rd53aReg PixPortal;
        //1
        Rd53aReg PixRegionCol;
        //2
        Rd53aReg PixRegionRow;
        //3
        Rd53aReg PixBroadcastEn;
        Rd53aReg PixAutoCol;
        Rd53aReg PixAutoRow;
        Rd53aReg PixBroadcastMask;
        //4
        Rd53aReg PixDefaultConfig;

        // Sync FE
        //5
        Rd53aReg SyncIbiasp1;
        //6
        Rd53aReg SyncIbiasp2;
        //7
        Rd53aReg SyncIbiasSf;
        //8
        Rd53aReg SyncIbiasKrum;
        //9
        Rd53aReg SyncIbiasDisc;
        //10
        Rd53aReg SyncIctrlSynct;
        //11
        Rd53aReg SyncVbl;
        //12
        Rd53aReg SyncVth;
        //13
        Rd53aReg SyncVrefKrum;
        //30 ***ooo
        Rd53aReg SyncAutoZero;
        Rd53aReg SyncSelC2F;
        Rd53aReg SyncSelC4F;
        Rd53aReg SyncFastTot;

        // Linear FE
        //14
        Rd53aReg LinPaInBias;
        //15
        Rd53aReg LinFcBias;
        //16
        Rd53aReg LinKrumCurr;
        //17
        Rd53aReg LinLdac;
        //18
        Rd53aReg LinComp;
        //19
        Rd53aReg LinRefKrum;
        //20
        Rd53aReg LinVth;

        // Diff FE
        //21
        Rd53aReg DiffPrmp;
        //22
        Rd53aReg DiffFol;
        //23
        Rd53aReg DiffPrecomp;
        //24
        Rd53aReg DiffComp;
        //25
        Rd53aReg DiffVff;
        //26
        Rd53aReg DiffVth1;
        //27
        Rd53aReg DiffVth2;
        //28
        Rd53aReg DiffLcc;
        //29
        Rd53aReg DiffLccEn;
        Rd53aReg DiffFbCapEn;

        // Power
        //31
        Rd53aReg SldoAnalogTrim;
        Rd53aReg SldoDigitalTrim;

        // Digital Matrix
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

        // Injection
        //39
        Rd53aReg InjEnDig;
        Rd53aReg InjAnaMode;
        Rd53aReg InjDelay;
        //41
        Rd53aReg InjVcalHigh;
        //42
        Rd53aReg InjVcalMed;
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

        // Digital Functions
        //40 ***ooo
        Rd53aReg ClkDelaySel;
        Rd53aReg ClkDelay;
        Rd53aReg CmdDelay;
        //43
        Rd53aReg ChSyncPhase;
        Rd53aReg ChSyncLock;
        Rd53aReg ChSyncUnlock;
        //44
        Rd53aReg GlobalPulseRt;

        // I/O
        //60
        Rd53aReg DebugConfig;
        //61
        Rd53aReg OutputDataReadDelay;
        Rd53aReg OutputSerType;
        Rd53aReg OutputActiveLanes;
        Rd53aReg OutputFmt;
        //62
        Rd53aReg OutPadConfig;
        //63
        Rd53aReg GpLvdsRoute;
        //64
        Rd53aReg CdrSelDelClk;
        Rd53aReg CdrPdSel;
        Rd53aReg CdrPdDel;
        Rd53aReg CdrEnGck;
        Rd53aReg CdrVcoGain;
        Rd53aReg CdrSelSerClk;
        //65
        Rd53aReg VcoBuffBias;
        //66
        Rd53aReg CdrCpIbias;
        //67
        Rd53aReg VcoIbias;
        //68
        Rd53aReg SerSelOut0;
        Rd53aReg SerSelOut1;
        Rd53aReg SerSelOut2;
        Rd53aReg SerSelOut3;
        //69
        Rd53aReg CmlInvTap;
        Rd53aReg CmlEnTap;
        Rd53aReg CmlEn;
        //70
        Rd53aReg CmlTapBias0;
        //71
        Rd53aReg CmlTapBias1;
        //72
        Rd53aReg CmlTapBias2;
        //73
        Rd53aReg AuroraCcWait;
        Rd53aReg AuroraCcSend;
        //74
        Rd53aReg AuroraCbWaitLow;
        Rd53aReg AuroraCbSend;
        //75
        Rd53aReg AuroraCbWaitHigh;
        //76
        Rd53aReg AuroraInitWait;
        //45
        Rd53aReg MonFrameSkip;
        //101
        Rd53aReg AutoReadA0;
        //102
        Rd53aReg AutoReadB0;
        //103
        Rd53aReg AutoReadA1;
        //104
        Rd53aReg AutoReadB1;
        //105
        Rd53aReg AutoReadA2;
        //106
        Rd53aReg AutoReadB2;
        //107
        Rd53aReg AutoReadA3;
        //108
        Rd53aReg AutoReadB3;

        // Test & Monitoring
        //77
        Rd53aReg MonitorEnable;
        Rd53aReg MonitorImonMux;
        Rd53aReg MonitorVmonMux;
        //78-81
        Rd53aReg HitOr0MaskSync;
        Rd53aReg HitOr1MaskSync;
        Rd53aReg HitOr2MaskSync;
        Rd53aReg HitOr3MaskSync;
        //82-89
        Rd53aReg HitOr0MaskLin0;
        Rd53aReg HitOr0MaskLin1;
        Rd53aReg HitOr1MaskLin0;
        Rd53aReg HitOr1MaskLin1;
        Rd53aReg HitOr2MaskLin0;
        Rd53aReg HitOr2MaskLin1;
        Rd53aReg HitOr3MaskLin0;
        Rd53aReg HitOr3MaskLin1;
        //90-97
        Rd53aReg HitOr0MaskDiff0;
        Rd53aReg HitOr0MaskDiff1;
        Rd53aReg HitOr1MaskDiff0;
        Rd53aReg HitOr1MaskDiff1;
        Rd53aReg HitOr2MaskDiff0;
        Rd53aReg HitOr2MaskDiff1;
        Rd53aReg HitOr3MaskDiff0;
        Rd53aReg HitOr3MaskDiff1;
        //98
        Rd53aReg AdcRefTrim;
        Rd53aReg AdcTrim;
        //99
        Rd53aReg SensorCfg0;
        Rd53aReg SensorCfg1;
        //109
        Rd53aReg RingOscEn;
        //110-117
        Rd53aReg RingOsc0;
        Rd53aReg RingOsc1;
        Rd53aReg RingOsc2;
        Rd53aReg RingOsc3;
        Rd53aReg RingOsc4;
        Rd53aReg RingOsc5;
        Rd53aReg RingOsc6;
        Rd53aReg RingOsc7;
        //118
        Rd53aReg BcCounter;
        //119
        Rd53aReg TrigCounter;
        //120
        Rd53aReg LockLossCounter;
        //121
        Rd53aReg BflipWarnCounter;
        //122
        Rd53aReg BflipErrCounter;
        //123
        Rd53aReg CmdErrCounter;
        //124-127
        Rd53aReg FifoFullCounter0;
        Rd53aReg FifoFullCounter1;
        Rd53aReg FifoFullCounter2;
        Rd53aReg FifoFullCounter3;
        //128
        Rd53aReg AiPixCol;
        //129
        Rd53aReg AiPixRow;
        //130-133
        Rd53aReg HitOrCounter0;
        Rd53aReg HitOrCounter1;
        Rd53aReg HitOrCounter2;
        Rd53aReg HitOrCounter3;
        //134
        Rd53aReg SkipTriggerCounter;
        //135
        Rd53aReg ErrMask;
        //136
        Rd53aReg AdcRead;
        //137
        Rd53aReg SelfTrigEn;

};
#endif

