#ifndef RD53BGLOBALCFG_H
#define RD53BGLOBALCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B library
// # Comment: RD53B global register
// # Date: May 2020
// ################################

#include <iostream>
#include <map>
#include <string>
#include <cmath>

#include "storage.hpp"

class Rd53bReg {
    public:
        Rd53bReg() {
            m_cfg = NULL;
            m_bOffset = 0;
            m_bits = 0;
            m_addr = 999;
        }

        virtual ~Rd53bReg() {}
        
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

        uint16_t applyMask(uint16_t value) {
            unsigned mask = (1<<m_bits)-1;
            return ((value >> m_bOffset) & mask);
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

class Rd53bDiffReg : public Rd53bReg {
    public:
        void init(Rd53bReg *arg_lowRef, Rd53bReg *arg_highRef, bool changeHigh) {
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
            highValue = highRef->read();
            lowValue = lowRef->read();

        }

        uint16_t read() const override {
            uint16_t lowValue = lowRef->read();
            uint16_t highValue = highRef->read();
            return highValue - lowValue;
        }
    private:
        Rd53bReg *lowRef;
        Rd53bReg *highRef;
        bool m_changeHigh;
};

class Rd53bGlobalCfg {
    public:
        Rd53bGlobalCfg();
        ~Rd53bGlobalCfg();

        void init();

        uint16_t getValue(Rd53bReg Rd53bGlobalCfg::*ref) const;
        uint16_t getValue(std::string name) const;
        void setValue(Rd53bReg Rd53bGlobalCfg::*ref, uint16_t val);
        void setValue(std::string name, uint16_t val);

        uint16_t& operator[](unsigned index);

        // TODO this not be public
        std::map<std::string, Rd53bReg Rd53bGlobalCfg::*> regMap;
        std::map<std::string, Rd53bReg Rd53bGlobalCfg::*> virtRegMap;
    protected:
        static constexpr unsigned numRegs = 138;
        std::array<uint16_t, numRegs> m_cfg;

        void toJson(json &j);
        void fromJson(json &j);
    private:
    public:

        //0
        Rd53bReg PixPortal;
        //1
        Rd53bReg PixRegionCol;
        //2
        Rd53bReg PixRegionRow;
        //3
        Rd53bReg PixBroadcast;
        Rd53bReg PixConfMode;
        Rd53bReg PixAutoRow;
        //4
        Rd53bReg PixDefaultConfig;
        //5
        Rd53bReg PixDefaultConfigB;
        //6
        Rd53bReg GcrDefaultConfig;
        //7
        Rd53bReg GcrDefaultConfigB;

        // Diff AFE
        //8
        Rd53bReg DiffPreampL;
        //9
        Rd53bReg DiffPreampR;
        //10
        Rd53bReg DiffPreampTL;
        //11
        Rd53bReg DiffPreampTR;
        //12
        Rd53bReg DiffPreampT;
        //13
        Rd53bReg DiffPreampM;
        //14
        Rd53bReg DiffPreComp;
        //15
        Rd53bReg DiffComp;
        //16
        Rd53bReg DiffVff;
        //17
        Rd53bReg DiffTh1L;
        //18
        Rd53bReg DiffTh1R;
        //19
        Rd53bReg DiffTh1M;
        //20
        Rd53bReg DiffTh2;
        //21
        Rd53bReg DiffLcc;
        //37
        Rd53bReg DiffLccEn;
        Rd53bReg DiffFbCapEn;

        // Lin AFE
        //22
        Rd53bReg LinPreampL;
        //23
        Rd53bReg LinPreampR;
        //24
        Rd53bReg LinPreampTL;
        //25
        Rd53bReg LinPreampTR;
        //26
        Rd53bReg LinPreampT;
        //27
        Rd53bReg LinPreampM;
        //28
        Rd53bReg LinFc;
        //29
        Rd53bReg LinKrumCurr;
        //30
        Rd53bReg LinRefKrum;
        //31
        Rd53bReg LinComp;
        //32
        Rd53bReg LinCompTa;
        //33
        Rd53bReg LinGdacL;
        //34
        Rd53bReg LinGdacR;
        //35
        Rd53bReg LinGdacM;
        //36
        Rd53bReg LinLdac;

        // Power
        //38
        Rd53bReg SldoEnUndershuntA;
        Rd53bReg SldoEnUndershuntD;
        Rd53bReg SldoTrimA;
        Rd53bReg SldoTrimD;

        // Pixel Matrix
        //39
        Rd53bReg EnCoreCol3;
        //40
        Rd53bReg EnCoreCol2;
        //41
        Rd53bReg EnCoreCol1;
        //42
        Rd53bReg EnCoreCol0;
        //43
        Rd53bReg RstCoreCol3;
        //44
        Rd53bReg RstCoreCol2;
        //45
        Rd53bReg RstCoreCol1;
        //46
        Rd53bReg RstCoreCol0;

        // Digital functions
        //47
        Rd53bReg TwoLevelTrig;
        Rd53bReg Latency;
        //48
        Rd53bReg SelfTrigEn;
        Rd53bReg SelfTrigDigThrEn;
        Rd53bReg SelfTrigDigThr;
        //49
        Rd53bReg SelfTrigDelay;
        Rd53bReg SelfTrigMulti;
        //50
        Rd53bReg SelfTrigPattern;
        //51
        Rd53bReg DataReadDelay;
        Rd53bReg ReadTrigLatency;
        //52
        Rd53bReg TruncTimeoutConf;
        //53
        Rd53bReg InjDigEn;
        Rd53bReg InjAnaMode;
        Rd53bReg InjFineDelay;
        //54
        Rd53bReg FineDelayClk;
        Rd53bReg FineDelayData;
        //55
        Rd53bReg InjVcalHigh;
        //56
        Rd53bReg InjVcalMed;
        //57
        Rd53bReg CapMeasEnPar;
        Rd53bReg CapMeasEn;
        Rd53bReg InjVcalRange;
        //58
        Rd53bReg CdrOverwriteLimit;
        Rd53bReg CdrPhaseDetSel;
        Rd53bReg CdrClkSel;
        //59
        Rd53bReg ChSyncLockThr;
        //60
        Rd53bReg GlobalPulseConf;
        //61
        Rd53bReg GlobalPulseWidth;
        //62
        Rd53bReg ServiceBlockEn;
        Rd53bReg ServiceBlockPeriod;
        //63
        Rd53bReg TotEnPtot;
        Rd53bReg TotEnPtoa;
        Rd53bReg TotEn80;
        Rd53bReg TotEn6b4b;
        Rd53bReg TotPtotLatency;
        //64
        Rd53bReg PtotCoreColEn3;
        //65
        Rd53bReg PtotCoreColEn2;
        //66
        Rd53bReg PtotCoreColEn1;
        //67
        Rd53bReg PtotCoreColEn0;
        //68
        Rd53bReg DataMergeInPol;
        Rd53bReg EnChipId;
        Rd53bReg DataMergeEnClkGate;
        Rd53bReg DataMergeSelClk;
        Rd53bReg DataMergeEn;
        Rd53bReg DataMergeEnBond;
        //69
        Rd53bReg DataMergeInMux3;
        Rd53bReg DataMergeInMux2;
        Rd53bReg DataMergeInMux1;
        Rd53bReg DataMergeInMux0;
        Rd53bReg DataMergeOutMux3;
        Rd53bReg DataMergeOutMux2;
        Rd53bReg DataMergeOutMux1;
        Rd53bReg DataMergeOutMux0;
        //70-73
        Rd53bReg EnCoreColCal3;
        Rd53bReg EnCoreColCal2;
        Rd53bReg EnCoreColCal1;
        Rd53bReg EnCoreColCal0;
        //74
        Rd53bReg DataEnBcid;
        Rd53bReg DataEnL1id;
        Rd53bReg DataEnEos;
        Rd53bReg NumOfEventsInStream;
        //75
        Rd53bReg DataEnBinaryRo;
        Rd53bReg DataEnRaw;
        Rd53bReg DataEnHitRemoval;
        Rd53bReg DataMaxHits;
        Rd53bReg DataEnIsoHitRemoval;
        Rd53bReg DataMaxTot;
        //76
        Rd53bReg EvenMask;
        //77
        Rd53bReg OddMask;
        //78
        Rd53bReg EfuseConfig;
        //79
        Rd53bReg EfuseWriteData1;
        //80
        Rd53bReg EfuseWriteData0;
        //81
        Rd53bReg AuroraEnPrbs;
        Rd53bReg AuroraActiveLanes;
        Rd53bReg AuroraCCWait;
        Rd53bReg AuroraCCSend;
        //82
        Rd53bReg AuroraCBWait1;
        //83
        Rd53bReg AuroraCBWait0;
        Rd53bReg AuroraCBSend;
        //84
        Rd53bReg AuroraInitWait;
        //85
        Rd53bReg GpValReg;
        Rd53bReg GpCmosEn;
        Rd53bReg GpCmosDs;
        Rd53bReg GpLvdsEn;
        Rd53bReg GpLvdsBias;
        //86
        Rd53bReg GpCmosRoute;
        //87
        Rd53bReg GpLvdsPad3;
        Rd53bReg GpLvdsPad2;
        //88
        Rd53bReg GpLvdsPad1;
        Rd53bReg GpLvdsPad0;
        //89
        Rd53bReg CdrCp;
        //90
        Rd53bReg CdrCpFd;
        //91
        Rd53bReg CdrCpBuff;
        //92
        Rd53bReg CdrVco;
        //93
        Rd53bReg CdrVcoBuff;
        //94
        Rd53bReg SerSelOut3;
        Rd53bReg SerSelOut2;
        Rd53bReg SerSelOut1;
        Rd53bReg SerSelOut0;
        //95
        Rd53bReg SerInvTap;
        Rd53bReg SerEnTap;
        Rd53bReg SerEnLane;
        //96
        Rd53bReg CmlBias2;
        //97
        Rd53bReg CmlBias1;
        //98
        Rd53bReg CmlBias0;
        //99
        Rd53bReg MonitorEnable;
        Rd53bReg MonitorI;
        Rd53bReg MonitorV;
        //100
        Rd53bReg ErrWngMask;
        //101
        Rd53bReg MonSensSldoDigEn;
        Rd53bReg MonSensSldoDigDem;
        Rd53bReg MonSensSldoDigSelBias;
        Rd53bReg MonSensSldoAnaEn;
        Rd53bReg MonSensSldoAnaDem;
        Rd53bReg MonSensSldoAnaSelBias;
        //102
        Rd53bReg MonSensAcbEn;
        Rd53bReg MonSensAcbDem;
        Rd53bReg MonSensAcbSelBias;
        //103
        Rd53bReg VrefRsensBot;
        Rd53bReg VrefRsensTop;
        Rd53bReg VrefIn;
        Rd53bReg MonAdcTrim;
        //104
        Rd53bReg NtcDac;
        //105-108
        Rd53bReg HitOrMask3;
        Rd53bReg HitOrMask2;
        Rd53bReg HitOrMask1;
        Rd53bReg HitOrMask0;
        //109-116
        Rd53bReg AutoRead0;
        Rd53bReg AutoRead1;
        Rd53bReg AutoRead2;
        Rd53bReg AutoRead3;
        Rd53bReg AutoRead4;
        Rd53bReg AutoRead5;
        Rd53bReg AutoRead6;
        Rd53bReg AutoRead7;
        //117
        Rd53bReg RingOscBClear;
        Rd53bReg RingOscBEnBl;
        Rd53bReg RingOscBEnBr;
        Rd53bReg RingOscBEnCapA;
        Rd53bReg RingOscBEnFf;
        Rd53bReg RingOscBEnLvt;
        Rd53bReg RingOscAClear;
        Rd53bReg RingOscAEn;
        //118
        Rd53bReg RingOscARoute;
        Rd53bReg RingOscBRoute;
        //119-120
        Rd53bReg RingOscAOut;
        Rd53bReg RingOscBOut;
        //121-123 RO
        Rd53bReg BcidCnt;
        Rd53bReg TrigCnt;
        Rd53bReg ReadTrigCnt;
        //124-128
        Rd53bReg LockLossCnt;
        Rd53bReg BitFlipWngCnt;
        Rd53bReg BitFlipErrCnt;
        Rd53bReg CmdErrCnt;
        Rd53bReg RdWrFifoErrCnt;
        //129
        Rd53bReg AiRegionRow;
        //130-133
        Rd53bReg HitOrCnt3;
        Rd53bReg HitOrCnt2;
        Rd53bReg HitOrCnt1;
        Rd53bReg HitOrCnt0;
        //134
        Rd53bReg SkippedTrigCnt;
        //135-136
        Rd53bReg EfuseReadData0;
        Rd53bReg EfuseReadData1;
        //137
        Rd53bReg MonitoringDataAdc;
        //138-201
        //SEU_notmr - not implemented, do not need to store
        //202-255
        //SEU - not implemented, do not need to store
        
        // Special regs
        Rd53bDiffReg InjVcalDiff;
};


#endif
