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
#include <utility>
#include <array>

#include "storage.hpp"
#include "Rd53Reg.h"

class Rd53bRegDefault : public Rd53Reg {
    public:

        void init(unsigned addr, uint16_t *cfg, const unsigned bOffset, const unsigned bits, const uint16_t value) {
            m_addr = addr;
            m_cfg = cfg;
            m_bOffset = bOffset;
            m_bits = bits;
            this->write(value);
        }

        void write(const uint16_t value) override {
            unsigned mask = (1<<m_bits)-1;
            *m_cfg = (*m_cfg&(~(mask<<m_bOffset))) | ((value&mask)<<m_bOffset);
        }

        uint16_t read() const override {
            unsigned mask = (1<<m_bits)-1;
            return ((*m_cfg >> m_bOffset) & mask);
        }
};

class Rd53bDiffReg : public Rd53Reg {
    public:
        void init(Rd53bRegDefault *arg_lowRef, Rd53bRegDefault *arg_highRef, bool changeHigh) {
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
        Rd53bRegDefault *lowRef;
        Rd53bRegDefault *highRef;
        bool m_changeHigh;
};

class Rd53bMultiReg : public Rd53Reg {
    public:
        void init(std::vector<Rd53bRegDefault*> list) {
            regList = std::move(list);
            m_cfg = nullptr;
            m_bOffset = 0;
            m_bits = 0;
            m_addr = 0;
            if (!regList.empty()) {
                // Only the first register will actually be writte in the chip
                m_addr = regList[0]->addr();
            }
        }

        void write(const uint16_t value) override {
            for (auto &reg: regList) {
                reg->write(value);
            }
        }

        uint16_t read() const override {
            if (regList.empty()) {
                return 0;
            }
            return regList[0]->read();
        }
    private:
        std::vector<Rd53bRegDefault*> regList;
};

class Rd53bGlobalCfg {
    public:
        Rd53bGlobalCfg();
        ~Rd53bGlobalCfg();

        void init();

        uint16_t getValue(Rd53bRegDefault Rd53bGlobalCfg::*ref) const;
        uint16_t getValue(std::string name) const;
        void setValue(Rd53bRegDefault Rd53bGlobalCfg::*ref, uint16_t val);
        void setValue(std::string name, uint16_t val);

        uint16_t& operator[](unsigned index);

        // TODO this not be public
        std::map<std::string, Rd53bRegDefault Rd53bGlobalCfg::*> regMap;
        std::map<std::string, Rd53bRegDefault Rd53bGlobalCfg::*> virtRegMap;
    protected:
        static constexpr unsigned numRegs = 138;
        std::array<uint16_t, numRegs> m_cfg;

        void writeConfig(json &j);
        void loadConfig(const json &j);
    private:
    public:

        //0
        Rd53bRegDefault PixPortal;
        //1
        Rd53bRegDefault PixRegionCol;
        //2
        Rd53bRegDefault PixRegionRow;
        //3
        Rd53bRegDefault PixBroadcast;
        Rd53bRegDefault PixConfMode;
        Rd53bRegDefault PixAutoRow;
        //4
        Rd53bRegDefault PixDefaultConfig;
        //5
        Rd53bRegDefault PixDefaultConfigB;
        //6
        Rd53bRegDefault GcrDefaultConfig;
        //7
        Rd53bRegDefault GcrDefaultConfigB;

        // Diff AFE
        //8
        Rd53bRegDefault DiffPreampL;
        //9
        Rd53bRegDefault DiffPreampR;
        //10
        Rd53bRegDefault DiffPreampTL;
        //11
        Rd53bRegDefault DiffPreampTR;
        //12
        Rd53bRegDefault DiffPreampT;
        //13
        Rd53bRegDefault DiffPreampM;
        //14
        Rd53bRegDefault DiffPreComp;
        //15
        Rd53bRegDefault DiffComp;
        //16
        Rd53bRegDefault DiffVff;
        //17
        Rd53bRegDefault DiffTh1L;
        //18
        Rd53bRegDefault DiffTh1R;
        //19
        Rd53bRegDefault DiffTh1M;
        //20
        Rd53bRegDefault DiffTh2;
        //21
        Rd53bRegDefault DiffLcc;
        //37
        Rd53bRegDefault DiffLccEn;
        Rd53bRegDefault DiffFbCapEn;

        // Lin AFE
        //22
        Rd53bRegDefault LinPreampL;
        //23
        Rd53bRegDefault LinPreampR;
        //24
        Rd53bRegDefault LinPreampTL;
        //25
        Rd53bRegDefault LinPreampTR;
        //26
        Rd53bRegDefault LinPreampT;
        //27
        Rd53bRegDefault LinPreampM;
        //28
        Rd53bRegDefault LinFc;
        //29
        Rd53bRegDefault LinKrumCurr;
        //30
        Rd53bRegDefault LinRefKrum;
        //31
        Rd53bRegDefault LinComp;
        //32
        Rd53bRegDefault LinCompTa;
        //33
        Rd53bRegDefault LinGdacL;
        //34
        Rd53bRegDefault LinGdacR;
        //35
        Rd53bRegDefault LinGdacM;
        //36
        Rd53bRegDefault LinLdac;

        // Power
        //38
        Rd53bRegDefault SldoEnUndershuntA;
        Rd53bRegDefault SldoEnUndershuntD;
        Rd53bRegDefault SldoTrimA;
        Rd53bRegDefault SldoTrimD;

        // Pixel Matrix
        //39
        Rd53bRegDefault EnCoreCol3;
        //40
        Rd53bRegDefault EnCoreCol2;
        //41
        Rd53bRegDefault EnCoreCol1;
        //42
        Rd53bRegDefault EnCoreCol0;
        //43
        Rd53bRegDefault RstCoreCol3;
        //44
        Rd53bRegDefault RstCoreCol2;
        //45
        Rd53bRegDefault RstCoreCol1;
        //46
        Rd53bRegDefault RstCoreCol0;

        // Digital functions
        //47
        Rd53bRegDefault TwoLevelTrig;
        Rd53bRegDefault Latency;
        //48
        Rd53bRegDefault SelfTrigEn;
        Rd53bRegDefault SelfTrigDigThrEn;
        Rd53bRegDefault SelfTrigDigThr;
        //49
        Rd53bRegDefault SelfTrigDelay;
        Rd53bRegDefault SelfTrigMulti;
        //50
        Rd53bRegDefault SelfTrigPattern;
        //51
        Rd53bRegDefault DataReadDelay;
        Rd53bRegDefault ReadTrigLatency;
        //52
        Rd53bRegDefault TruncTimeoutConf;
        //53
        Rd53bRegDefault InjDigEn;
        Rd53bRegDefault InjAnaMode;
        Rd53bRegDefault InjFineDelay;
        //54
        Rd53bRegDefault FineDelayClk;
        Rd53bRegDefault FineDelayData;
        //55
        Rd53bRegDefault InjVcalHigh;
        //56
        Rd53bRegDefault InjVcalMed;
        //57
        Rd53bRegDefault CapMeasEnPar;
        Rd53bRegDefault CapMeasEn;
        Rd53bRegDefault InjVcalRange;
        //58
        Rd53bRegDefault CdrOverwriteLimit;
        Rd53bRegDefault CdrPhaseDetSel;
        Rd53bRegDefault CdrClkSel;
        //59
        Rd53bRegDefault ChSyncLockThr;
        //60
        Rd53bRegDefault GlobalPulseConf;
        //61
        Rd53bRegDefault GlobalPulseWidth;
        //62
        Rd53bRegDefault ServiceBlockEn;
        Rd53bRegDefault ServiceBlockPeriod;
        //63
        Rd53bRegDefault TotEnPtot;
        Rd53bRegDefault TotEnPtoa;
        Rd53bRegDefault TotEn80;
        Rd53bRegDefault TotEn6b4b;
        Rd53bRegDefault TotPtotLatency;
        //64
        Rd53bRegDefault PtotCoreColEn3;
        //65
        Rd53bRegDefault PtotCoreColEn2;
        //66
        Rd53bRegDefault PtotCoreColEn1;
        //67
        Rd53bRegDefault PtotCoreColEn0;
        //68
        Rd53bRegDefault DataMergeInPol;
        Rd53bRegDefault EnChipId;
        Rd53bRegDefault DataMergeEnClkGate;
        Rd53bRegDefault DataMergeSelClk;
        Rd53bRegDefault DataMergeEn;
        Rd53bRegDefault DataMergeEnBond;
        //69
        Rd53bRegDefault DataMergeInMux3;
        Rd53bRegDefault DataMergeInMux2;
        Rd53bRegDefault DataMergeInMux1;
        Rd53bRegDefault DataMergeInMux0;
        Rd53bRegDefault DataMergeOutMux3;
        Rd53bRegDefault DataMergeOutMux2;
        Rd53bRegDefault DataMergeOutMux1;
        Rd53bRegDefault DataMergeOutMux0;
        //70-73
        Rd53bRegDefault EnCoreColCal3;
        Rd53bRegDefault EnCoreColCal2;
        Rd53bRegDefault EnCoreColCal1;
        Rd53bRegDefault EnCoreColCal0;
        //74
        Rd53bRegDefault DataEnBcid;
        Rd53bRegDefault DataEnL1id;
        Rd53bRegDefault DataEnEos;
        Rd53bRegDefault NumOfEventsInStream;
        //75
        Rd53bRegDefault DataEnBinaryRo;
        Rd53bRegDefault DataEnRaw;
        Rd53bRegDefault DataEnHitRemoval;
        Rd53bRegDefault DataMaxHits;
        Rd53bRegDefault DataEnIsoHitRemoval;
        Rd53bRegDefault DataMaxTot;
        //76
        Rd53bRegDefault EvenMask;
        //77
        Rd53bRegDefault OddMask;
        //78
        Rd53bRegDefault EfuseConfig;
        //79
        Rd53bRegDefault EfuseWriteData1;
        //80
        Rd53bRegDefault EfuseWriteData0;
        //81
        Rd53bRegDefault AuroraEnPrbs;
        Rd53bRegDefault AuroraActiveLanes;
        Rd53bRegDefault AuroraCCWait;
        Rd53bRegDefault AuroraCCSend;
        //82
        Rd53bRegDefault AuroraCBWait1;
        //83
        Rd53bRegDefault AuroraCBWait0;
        Rd53bRegDefault AuroraCBSend;
        //84
        Rd53bRegDefault AuroraInitWait;
        //85
        Rd53bRegDefault GpValReg;
        Rd53bRegDefault GpCmosEn;
        Rd53bRegDefault GpCmosDs;
        Rd53bRegDefault GpLvdsEn;
        Rd53bRegDefault GpLvdsBias;
        //86
        Rd53bRegDefault GpCmosRoute;
        //87
        Rd53bRegDefault GpLvdsPad3;
        Rd53bRegDefault GpLvdsPad2;
        //88
        Rd53bRegDefault GpLvdsPad1;
        Rd53bRegDefault GpLvdsPad0;
        //89
        Rd53bRegDefault CdrCp;
        //90
        Rd53bRegDefault CdrCpFd;
        //91
        Rd53bRegDefault CdrCpBuff;
        //92
        Rd53bRegDefault CdrVco;
        //93
        Rd53bRegDefault CdrVcoBuff;
        //94
        Rd53bRegDefault SerSelOut3;
        Rd53bRegDefault SerSelOut2;
        Rd53bRegDefault SerSelOut1;
        Rd53bRegDefault SerSelOut0;
        //95
        Rd53bRegDefault SerInvTap;
        Rd53bRegDefault SerEnTap;
        Rd53bRegDefault SerEnLane;
        //96
        Rd53bRegDefault CmlBias2;
        //97
        Rd53bRegDefault CmlBias1;
        //98
        Rd53bRegDefault CmlBias0;
        //99
        Rd53bRegDefault MonitorEnable;
        Rd53bRegDefault MonitorI;
        Rd53bRegDefault MonitorV;
        //100
        Rd53bRegDefault ErrWngMask;
        //101
        Rd53bRegDefault MonSensSldoDigEn;
        Rd53bRegDefault MonSensSldoDigDem;
        Rd53bRegDefault MonSensSldoDigSelBias;
        Rd53bRegDefault MonSensSldoAnaEn;
        Rd53bRegDefault MonSensSldoAnaDem;
        Rd53bRegDefault MonSensSldoAnaSelBias;
        //102
        Rd53bRegDefault MonSensAcbEn;
        Rd53bRegDefault MonSensAcbDem;
        Rd53bRegDefault MonSensAcbSelBias;
        //103
        Rd53bRegDefault VrefRsensBot;
        Rd53bRegDefault VrefRsensTop;
        Rd53bRegDefault VrefIn;
        Rd53bRegDefault MonAdcTrim;
        //104
        Rd53bRegDefault NtcDac;
        //105-108
        Rd53bRegDefault HitOrMask3;
        Rd53bRegDefault HitOrMask2;
        Rd53bRegDefault HitOrMask1;
        Rd53bRegDefault HitOrMask0;
        //109-116
        Rd53bRegDefault AutoRead0;
        Rd53bRegDefault AutoRead1;
        Rd53bRegDefault AutoRead2;
        Rd53bRegDefault AutoRead3;
        Rd53bRegDefault AutoRead4;
        Rd53bRegDefault AutoRead5;
        Rd53bRegDefault AutoRead6;
        Rd53bRegDefault AutoRead7;
        //117
        Rd53bRegDefault RingOscBClear;
        Rd53bRegDefault RingOscBEnBl;
        Rd53bRegDefault RingOscBEnBr;
        Rd53bRegDefault RingOscBEnCapA;
        Rd53bRegDefault RingOscBEnFf;
        Rd53bRegDefault RingOscBEnLvt;
        Rd53bRegDefault RingOscAClear;
        Rd53bRegDefault RingOscAEn;
        //118
        Rd53bRegDefault RingOscARoute;
        Rd53bRegDefault RingOscBRoute;
        //119-120
        Rd53bRegDefault RingOscAOut;
        Rd53bRegDefault RingOscBOut;
        //121-123 RO
        Rd53bRegDefault BcidCnt;
        Rd53bRegDefault TrigCnt;
        Rd53bRegDefault ReadTrigCnt;
        //124-128
        Rd53bRegDefault LockLossCnt;
        Rd53bRegDefault BitFlipWngCnt;
        Rd53bRegDefault BitFlipErrCnt;
        Rd53bRegDefault CmdErrCnt;
        Rd53bRegDefault RdWrFifoErrCnt;
        //129
        Rd53bRegDefault AiRegionRow;
        //130-133
        Rd53bRegDefault HitOrCnt3;
        Rd53bRegDefault HitOrCnt2;
        Rd53bRegDefault HitOrCnt1;
        Rd53bRegDefault HitOrCnt0;
        //134
        Rd53bRegDefault SkippedTrigCnt;
        //135-136
        Rd53bRegDefault EfuseReadData0;
        Rd53bRegDefault EfuseReadData1;
        //137
        Rd53bRegDefault MonitoringDataAdc;
        //138-201
        //SEU_notmr - not implemented, do not need to store
        //202-255
        //SEU - not implemented, do not need to store
        
        // Special regs
        Rd53bDiffReg InjVcalDiff;
        Rd53bMultiReg DiffTh1;
};


#endif
