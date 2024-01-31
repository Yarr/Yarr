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
#include <array>

#include "Rd53Reg.h"
#include "storage.hpp"

class Rd53aRegDefault : public Rd53Reg {
    public:

        void init(unsigned addr, uint16_t *cfg, const unsigned bOffset, const unsigned bits, const uint16_t value) {
            m_addr = addr;
            m_cfg = cfg;
            m_bOffset = bOffset;
            m_bits = bits;
            this->write(value);
        }

        void write(const uint16_t value) final {
            unsigned mask = (1<<m_bits)-1;
            *m_cfg = (*m_cfg&(~(mask<<m_bOffset))) | ((value&mask)<<m_bOffset);
        }

        uint16_t read() const final {
            unsigned mask = (1<<m_bits)-1;
            return ((*m_cfg >> m_bOffset) & mask);
        }
};

class Rd53aDiffReg : public Rd53Reg {
    public:
        void init(Rd53aRegDefault *arg_lowRef, Rd53aRegDefault *arg_highRef, bool changeHigh) {
            lowRef = arg_lowRef;
            highRef = arg_highRef;
            m_cfg = nullptr; // Not needed
            m_bOffset = 0; // Not needed
            m_bits = 0; // Not needed
            m_changeHigh = changeHigh;
            if (m_changeHigh) {
                m_addr = highRef->addr(); // Write register asks for the address, only want to modify highRef
            } else {
                m_addr = lowRef->addr();
            }

        }

        void write(const uint16_t value) final {
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

        uint16_t read() const final {
            uint16_t lowValue = lowRef->read();
            uint16_t highValue = highRef->read();
            return highValue - lowValue;
        }
    private:
        Rd53aRegDefault *lowRef;
        Rd53aRegDefault *highRef;
        bool m_changeHigh;
};

class Rd53aGlobalCfg {
    public:
        static constexpr unsigned numRegs = 138;
        std::array<uint16_t, numRegs> m_cfg{};

        Rd53aGlobalCfg();
        ~Rd53aGlobalCfg();
        void init();

        uint16_t getValue(Rd53aRegDefault Rd53aGlobalCfg::*ref) {
            return (this->*ref).read();
        }

        uint16_t getValue(const std::string& name) {
            if (regMap.find(name) != regMap.end()) {
                return (this->*regMap[name]).read();
            } else {
                std::cerr << " --> Error: Could not find register \"" << name << "\"" << std::endl;
            }
            return 0;
        }

        void setValue(Rd53aRegDefault Rd53aGlobalCfg::*ref, uint16_t val) {
            (this->*ref).write(val);
        }

        void setValue(const std::string& name, uint16_t val) {
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
        void writeConfig(json &j);
        void loadConfig(const json &j);
    private:
    public:
        std::map<std::string, Rd53Reg Rd53aGlobalCfg::*> regMap;
        //Special diff registers
        Rd53aDiffReg InjVcalDiff;

        //0
        Rd53aRegDefault PixPortal;
        //1
        Rd53aRegDefault PixRegionCol;
        //2
        Rd53aRegDefault PixRegionRow;
        //3
        Rd53aRegDefault PixBroadcastEn;
        Rd53aRegDefault PixAutoCol;
        Rd53aRegDefault PixAutoRow;
        Rd53aRegDefault PixBroadcastMask;
        //4
        Rd53aRegDefault PixDefaultConfig;

        // Sync FE
        //5
        Rd53aRegDefault SyncIbiasp1;
        //6
        Rd53aRegDefault SyncIbiasp2;
        //7
        Rd53aRegDefault SyncIbiasSf;
        //8
        Rd53aRegDefault SyncIbiasKrum;
        //9
        Rd53aRegDefault SyncIbiasDisc;
        //10
        Rd53aRegDefault SyncIctrlSynct;
        //11
        Rd53aRegDefault SyncVbl;
        //12
        Rd53aRegDefault SyncVth;
        //13
        Rd53aRegDefault SyncVrefKrum;
        //30 ***ooo
        Rd53aRegDefault SyncAutoZero;
        Rd53aRegDefault SyncSelC2F;
        Rd53aRegDefault SyncSelC4F;
        Rd53aRegDefault SyncFastTot;

        // Linear FE
        //14
        Rd53aRegDefault LinPaInBias;
        //15
        Rd53aRegDefault LinFcBias;
        //16
        Rd53aRegDefault LinKrumCurr;
        //17
        Rd53aRegDefault LinLdac;
        //18
        Rd53aRegDefault LinComp;
        //19
        Rd53aRegDefault LinRefKrum;
        //20
        Rd53aRegDefault LinVth;

        // Diff FE
        //21
        Rd53aRegDefault DiffPrmp;
        //22
        Rd53aRegDefault DiffFol;
        //23
        Rd53aRegDefault DiffPrecomp;
        //24
        Rd53aRegDefault DiffComp;
        //25
        Rd53aRegDefault DiffVff;
        //26
        Rd53aRegDefault DiffVth1;
        //27
        Rd53aRegDefault DiffVth2;
        //28
        Rd53aRegDefault DiffLcc;
        //29
        Rd53aRegDefault DiffLccEn;
        Rd53aRegDefault DiffFbCapEn;

        // Power
        //31
        Rd53aRegDefault SldoAnalogTrim;
        Rd53aRegDefault SldoDigitalTrim;

        // Digital Matrix
        //32
        Rd53aRegDefault EnCoreColSync;
        //33
        Rd53aRegDefault EnCoreColLin1;
        //34
        Rd53aRegDefault EnCoreColLin2;
        //35
        Rd53aRegDefault EnCoreColDiff1;
        //36
        Rd53aRegDefault EnCoreColDiff2;
        //37
        Rd53aRegDefault LatencyConfig;
        //38
        Rd53aRegDefault WrSyncDelaySync;

        // Injection
        //39
        Rd53aRegDefault InjEnDig;
        Rd53aRegDefault InjAnaMode;
        Rd53aRegDefault InjDelay;
        //41
        Rd53aRegDefault InjVcalHigh;
        //42
        Rd53aRegDefault InjVcalMed;
        //46
        Rd53aRegDefault CalColprSync1;
        //47
        Rd53aRegDefault CalColprSync2;
        //48
        Rd53aRegDefault CalColprSync3;
        //49
        Rd53aRegDefault CalColprSync4;
        //50
        Rd53aRegDefault CalColprLin1;
        //51
        Rd53aRegDefault CalColprLin2;
        //52
        Rd53aRegDefault CalColprLin3;
        //53
        Rd53aRegDefault CalColprLin4;
        //54
        Rd53aRegDefault CalColprLin5;
        //55
        Rd53aRegDefault CalColprDiff1;
        //56
        Rd53aRegDefault CalColprDiff2;
        //57
        Rd53aRegDefault CalColprDiff3;
        //58
        Rd53aRegDefault CalColprDiff4;
        //59
        Rd53aRegDefault CalColprDiff5;

        // Digital Functions
        //40 ***ooo
        Rd53aRegDefault ClkDelaySel;
        Rd53aRegDefault ClkDelay;
        Rd53aRegDefault CmdDelay;
        //43
        Rd53aRegDefault ChSyncPhase;
        Rd53aRegDefault ChSyncLock;
        Rd53aRegDefault ChSyncUnlock;
        //44
        Rd53aRegDefault GlobalPulseRt;

        // I/O
        //60
        Rd53aRegDefault DebugConfig;
        //61
        Rd53aRegDefault OutputDataReadDelay;
        Rd53aRegDefault OutputSerType;
        Rd53aRegDefault OutputActiveLanes;
        Rd53aRegDefault OutputFmt;
        //62
        Rd53aRegDefault OutPadConfig;
        //63
        Rd53aRegDefault GpLvdsRoute;
        //64
        Rd53aRegDefault CdrSelDelClk;
        Rd53aRegDefault CdrPdSel;
        Rd53aRegDefault CdrPdDel;
        Rd53aRegDefault CdrEnGck;
        Rd53aRegDefault CdrVcoGain;
        Rd53aRegDefault CdrSelSerClk;
        //65
        Rd53aRegDefault VcoBuffBias;
        //66
        Rd53aRegDefault CdrCpIbias;
        //67
        Rd53aRegDefault VcoIbias;
        //68
        Rd53aRegDefault SerSelOut0;
        Rd53aRegDefault SerSelOut1;
        Rd53aRegDefault SerSelOut2;
        Rd53aRegDefault SerSelOut3;
        //69
        Rd53aRegDefault CmlInvTap;
        Rd53aRegDefault CmlEnTap;
        Rd53aRegDefault CmlEn;
        //70
        Rd53aRegDefault CmlTapBias0;
        //71
        Rd53aRegDefault CmlTapBias1;
        //72
        Rd53aRegDefault CmlTapBias2;
        //73
        Rd53aRegDefault AuroraCcWait;
        Rd53aRegDefault AuroraCcSend;
        //74
        Rd53aRegDefault AuroraCbWaitLow;
        Rd53aRegDefault AuroraCbSend;
        //75
        Rd53aRegDefault AuroraCbWaitHigh;
        //76
        Rd53aRegDefault AuroraInitWait;
        //45
        Rd53aRegDefault MonFrameSkip;
        //101
        Rd53aRegDefault AutoReadA0;
        //102
        Rd53aRegDefault AutoReadB0;
        //103
        Rd53aRegDefault AutoReadA1;
        //104
        Rd53aRegDefault AutoReadB1;
        //105
        Rd53aRegDefault AutoReadA2;
        //106
        Rd53aRegDefault AutoReadB2;
        //107
        Rd53aRegDefault AutoReadA3;
        //108
        Rd53aRegDefault AutoReadB3;

        // Test & Monitoring
        //77
        Rd53aRegDefault MonitorEnable;
        Rd53aRegDefault MonitorImonMux;
        Rd53aRegDefault MonitorVmonMux;
        //78-81
        Rd53aRegDefault HitOr0MaskSync;
        Rd53aRegDefault HitOr1MaskSync;
        Rd53aRegDefault HitOr2MaskSync;
        Rd53aRegDefault HitOr3MaskSync;
        //82-89
        Rd53aRegDefault HitOr0MaskLin0;
        Rd53aRegDefault HitOr0MaskLin1;
        Rd53aRegDefault HitOr1MaskLin0;
        Rd53aRegDefault HitOr1MaskLin1;
        Rd53aRegDefault HitOr2MaskLin0;
        Rd53aRegDefault HitOr2MaskLin1;
        Rd53aRegDefault HitOr3MaskLin0;
        Rd53aRegDefault HitOr3MaskLin1;
        //90-97
        Rd53aRegDefault HitOr0MaskDiff0;
        Rd53aRegDefault HitOr0MaskDiff1;
        Rd53aRegDefault HitOr1MaskDiff0;
        Rd53aRegDefault HitOr1MaskDiff1;
        Rd53aRegDefault HitOr2MaskDiff0;
        Rd53aRegDefault HitOr2MaskDiff1;
        Rd53aRegDefault HitOr3MaskDiff0;
        Rd53aRegDefault HitOr3MaskDiff1;
        //98
        Rd53aRegDefault AdcRefTrim;
        Rd53aRegDefault AdcTrim;
        //99
        Rd53aRegDefault SensorCfg0;
        Rd53aRegDefault SensorCfg1;
        //109
        Rd53aRegDefault RingOscEn;
        //110-117
        Rd53aRegDefault RingOsc0;
        Rd53aRegDefault RingOsc1;
        Rd53aRegDefault RingOsc2;
        Rd53aRegDefault RingOsc3;
        Rd53aRegDefault RingOsc4;
        Rd53aRegDefault RingOsc5;
        Rd53aRegDefault RingOsc6;
        Rd53aRegDefault RingOsc7;
        //118
        Rd53aRegDefault BcCounter;
        //119
        Rd53aRegDefault TrigCounter;
        //120
        Rd53aRegDefault LockLossCounter;
        //121
        Rd53aRegDefault BflipWarnCounter;
        //122
        Rd53aRegDefault BflipErrCounter;
        //123
        Rd53aRegDefault CmdErrCounter;
        //124-127
        Rd53aRegDefault FifoFullCounter0;
        Rd53aRegDefault FifoFullCounter1;
        Rd53aRegDefault FifoFullCounter2;
        Rd53aRegDefault FifoFullCounter3;
        //128
        Rd53aRegDefault AiPixCol;
        //129
        Rd53aRegDefault AiPixRow;
        //130-133
        Rd53aRegDefault HitOrCounter0;
        Rd53aRegDefault HitOrCounter1;
        Rd53aRegDefault HitOrCounter2;
        Rd53aRegDefault HitOrCounter3;
        //134
        Rd53aRegDefault SkipTriggerCounter;
        //135
        Rd53aRegDefault ErrMask;
        //136
        Rd53aRegDefault AdcRead;
        //137
        Rd53aRegDefault SelfTrigEn;

};
#endif

