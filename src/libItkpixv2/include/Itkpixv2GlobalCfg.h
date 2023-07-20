#ifndef ITKPIXV2GLOBALCFG_H
#define ITKPIXV2GLOBALCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 library
// # Comment: ITkPixV2 global register
// # Date: Jul 2023
// ################################

#include <iostream>
#include <map>
#include <string>
#include <cmath>
#include <utility>

#include "storage.hpp"
#include "Rd53Reg.h"

class Itkpixv2RegDefault : public Rd53Reg {
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

class Itkpixv2DiffReg : public Rd53Reg {
    public:
        void init(Itkpixv2RegDefault *arg_lowRef, Itkpixv2RegDefault *arg_highRef, bool changeHigh) {
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
        Itkpixv2RegDefault *lowRef;
        Itkpixv2RegDefault *highRef;
        bool m_changeHigh;
};

class Itkpixv2MultiReg : public Rd53Reg {
    public:
        void init(std::vector<Itkpixv2RegDefault*> list) {
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
        std::vector<Itkpixv2RegDefault*> regList;
};

class Itkpixv2GlobalCfg {
    public:
        Itkpixv2GlobalCfg();
        ~Itkpixv2GlobalCfg();

        void init();

        uint16_t getValue(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref) const;
        uint16_t getValue(std::string name) const;
        void setValue(Itkpixv2RegDefault Itkpixv2GlobalCfg::*ref, uint16_t val);
        void setValue(std::string name, uint16_t val);

        uint16_t& operator[](unsigned index);

        // TODO this not be public
        std::map<std::string, Itkpixv2RegDefault Itkpixv2GlobalCfg::*> regMap;
        std::map<std::string, Itkpixv2RegDefault Itkpixv2GlobalCfg::*> virtRegMap;
    protected:
        static constexpr unsigned numRegs = 138;
        std::array<uint16_t, numRegs> m_cfg;

        void writeConfig(json &j);
        void loadConfig(const json &j);
    private:
    public:

        //0
        Itkpixv2RegDefault PixPortal;
        //1
        Itkpixv2RegDefault PixRegionCol;
        //2
        Itkpixv2RegDefault PixRegionRow;
        //3
        Itkpixv2RegDefault PixEnSeuCount;
        Itkpixv2RegDefault PixHitSampleMode;
        Itkpixv2RegDefault PixBroadcast;
        Itkpixv2RegDefault PixConfMode;
        Itkpixv2RegDefault PixAutoRow;
        //4
        Itkpixv2RegDefault PixDefaultConfig;
        //5
        Itkpixv2RegDefault PixDefaultConfigB;
        //6
        Itkpixv2RegDefault GcrDefaultConfig;
        //7
        Itkpixv2RegDefault GcrDefaultConfigB;

        // Diff AFE
        //8
        Itkpixv2RegDefault DiffPreampL;
        //9
        Itkpixv2RegDefault DiffPreampR;
        //10
        Itkpixv2RegDefault DiffPreampTL;
        //11
        Itkpixv2RegDefault DiffPreampTR;
        //12
        Itkpixv2RegDefault DiffPreampT;
        //13
        Itkpixv2RegDefault DiffPreampM;
        //14
        Itkpixv2RegDefault DiffPreComp;
        //15
        Itkpixv2RegDefault DiffComp;
        //16
        Itkpixv2RegDefault DiffVff;
        //17
        Itkpixv2RegDefault DiffTh1L;
        //18
        Itkpixv2RegDefault DiffTh1R;
        //19
        Itkpixv2RegDefault DiffTh1M;
        //20
        Itkpixv2RegDefault DiffTh2;
        //21
        Itkpixv2RegDefault DiffLcc;
        //37
        Itkpixv2RegDefault DiffLccEn;
        Itkpixv2RegDefault DiffFbCapEn;

        // Lin AFE
        //22
        Itkpixv2RegDefault LinPreampL;
        //23
        Itkpixv2RegDefault LinPreampR;
        //24
        Itkpixv2RegDefault LinPreampTL;
        //25
        Itkpixv2RegDefault LinPreampTR;
        //26
        Itkpixv2RegDefault LinPreampT;
        //27
        Itkpixv2RegDefault LinPreampM;
        //28
        Itkpixv2RegDefault LinFc;
        //29
        Itkpixv2RegDefault LinKrumCurr;
        //30
        Itkpixv2RegDefault LinRefKrum;
        //31
        Itkpixv2RegDefault LinComp;
        //32
        Itkpixv2RegDefault LinCompTa;
        //33
        Itkpixv2RegDefault LinGdacL;
        //34
        Itkpixv2RegDefault LinGdacR;
        //35
        Itkpixv2RegDefault LinGdacM;
        //36
        Itkpixv2RegDefault LinLdac;

        // Power
        //38
        Itkpixv2RegDefault SldoEnUndershuntA;
        Itkpixv2RegDefault SldoEnUndershuntD;
        Itkpixv2RegDefault SldoTrimA;
        Itkpixv2RegDefault SldoTrimD;

        // Pixel Matrix
        //39
        Itkpixv2RegDefault EnCoreCol3;
        //40
        Itkpixv2RegDefault EnCoreCol2;
        //41
        Itkpixv2RegDefault EnCoreCol1;
        //42
        Itkpixv2RegDefault EnCoreCol0;
        //43
        Itkpixv2RegDefault RstCoreCol3;
        //44
        Itkpixv2RegDefault RstCoreCol2;
        //45
        Itkpixv2RegDefault RstCoreCol1;
        //46
        Itkpixv2RegDefault RstCoreCol0;

        // Digital functions
        //47
        Itkpixv2RegDefault TwoLevelTrig;
        Itkpixv2RegDefault Latency;
        //48
        Itkpixv2RegDefault SelfTrigEn;
        Itkpixv2RegDefault SelfTrigDigThrEn;
        Itkpixv2RegDefault SelfTrigDigThr;
        //49
        Itkpixv2RegDefault SelfTrigDelay;
        Itkpixv2RegDefault SelfTrigMulti;
        //50
        Itkpixv2RegDefault SelfTrigPattern;
        //51
        Itkpixv2RegDefault SelfTrigDeadtime;
        //52
        Itkpixv2RegDefault DataReadDelay;
        Itkpixv2RegDefault ReadTrigLatency;
        //53
        Itkpixv2RegDefault TruncTimeoutConf;
        //54
        Itkpixv2RegDefault InjDigEn;
        Itkpixv2RegDefault InjAnaMode;
        Itkpixv2RegDefault InjFineDelay;
        //55
        Itkpixv2RegDefault FineDelayClk;
        Itkpixv2RegDefault FineDelayData;
        //56
        Itkpixv2RegDefault InjVcalHigh;
        //57
        Itkpixv2RegDefault InjVcalMed;
        //58
        Itkpixv2RegDefault CapMeasEnPar;
        Itkpixv2RegDefault CapMeasEn;
        Itkpixv2RegDefault InjVcalRange;
        //59
        Itkpixv2RegDefault CdrOverwriteLimit;
        Itkpixv2RegDefault CdrPhaseDetSel;
        Itkpixv2RegDefault CdrClkSel;
        //60
        Itkpixv2RegDefault Clk40En;
        Itkpixv2RegDefault Clk160En;
        Itkpixv2RegDefault ClkDataMergeEn;
        Itkpixv2RegDefault ClkAuroraEn;
        //61
        Itkpixv2RegDefault ChSyncLockThr;
        //62
        Itkpixv2RegDefault GlobalPulseConf;
        //63
        Itkpixv2RegDefault GlobalPulseWidth;
        //64
        Itkpixv2RegDefault ServiceBlockEn;
        Itkpixv2RegDefault ServiceBlockPeriod;
        //65
        Itkpixv2RegDefault TotEnPtot;
        Itkpixv2RegDefault TotEnPtoa;
        Itkpixv2RegDefault TotEn80;
        Itkpixv2RegDefault TotEn6b4b;
        Itkpixv2RegDefault TotPtotLatency;
        //66
        Itkpixv2RegDefault PtotCoreColEn3;
        //67
        Itkpixv2RegDefault PtotCoreColEn2;
        //68
        Itkpixv2RegDefault PtotCoreColEn1;
        //69
        Itkpixv2RegDefault PtotCoreColEn0;
        //70
        Itkpixv2RegDefault DataMergeInPol;
        Itkpixv2RegDefault EnChipId;
        Itkpixv2RegDefault DataMergeEnClkGate;
        Itkpixv2RegDefault DataMergeSelClk;
        Itkpixv2RegDefault DataMergeEn;
        Itkpixv2RegDefault DataMergeEnBond;
        Itkpixv2RegDefault DataMergeGpoSel;
        //71
        Itkpixv2RegDefault DataMergeInMux3;
        Itkpixv2RegDefault DataMergeInMux2;
        Itkpixv2RegDefault DataMergeInMux1;
        Itkpixv2RegDefault DataMergeInMux0;
        Itkpixv2RegDefault DataMergeOutMux3;
        Itkpixv2RegDefault DataMergeOutMux2;
        Itkpixv2RegDefault DataMergeOutMux1;
        Itkpixv2RegDefault DataMergeOutMux0;
        //72-75
        Itkpixv2RegDefault EnCoreColCal3;
        Itkpixv2RegDefault EnCoreColCal2;
        Itkpixv2RegDefault EnCoreColCal1;
        Itkpixv2RegDefault EnCoreColCal0;
        //76
        Itkpixv2RegDefault DataEnCrc;
        Itkpixv2RegDefault DataEnBcid;
        Itkpixv2RegDefault DataEnL1id;
        Itkpixv2RegDefault NumOfEventsInStream;
        //77
        Itkpixv2RegDefault DataEnBinaryRo;
        Itkpixv2RegDefault DataEnRaw;
        Itkpixv2RegDefault DataMaxHits;
        Itkpixv2RegDefault DataMaxTot;
        //78-81
        Itkpixv2RegDefault EnHitsRemoval3;
        Itkpixv2RegDefault EnHitsRemoval2;
        Itkpixv2RegDefault EnHitsRemoval1;
        Itkpixv2RegDefault EnHitsRemoval0;
        //82-85
        Itkpixv2RegDefault EnIsoHitsRemoval3;
        Itkpixv2RegDefault EnIsoHitsRemoval2;
        Itkpixv2RegDefault EnIsoHitsRemoval1;
        Itkpixv2RegDefault EnIsoHitsRemoval0;
        //86
        Itkpixv2RegDefault EvenMask;
        //87
        Itkpixv2RegDefault OddMask;
        //88
        Itkpixv2RegDefault EfuseConfig;
        //89
        Itkpixv2RegDefault EfuseWriteData1;
        //90
        Itkpixv2RegDefault EfuseWriteData0;
        //91
        Itkpixv2RegDefault DataMergeFixedMode;
        Itkpixv2RegDefault DataMergeManMode;
        Itkpixv2RegDefault DataMergeManChoice;
        //92
        Itkpixv2RegDefault AuroraSendAlt;
        Itkpixv2RegDefault AuroraEnPrbs;
        Itkpixv2RegDefault AuroraActiveLanes;
        Itkpixv2RegDefault AuroraCCWait;
        Itkpixv2RegDefault AuroraCCSend;
        //93
        Itkpixv2RegDefault AuroraCBWait1;
        //94
        Itkpixv2RegDefault AuroraCBWait0;
        Itkpixv2RegDefault AuroraCBSend;
        //95
        Itkpixv2RegDefault AuroraInitWait;
        //96-97
        Itkpixv2RegDefault AuroraAltOutput0;
        Itkpixv2RegDefault AuroraAltOutput1;
        //98
        Itkpixv2RegDefault GpValReg;
        Itkpixv2RegDefault GpCmosEn;
        Itkpixv2RegDefault GpCmosDs;
        Itkpixv2RegDefault GpLvdsEn;
        Itkpixv2RegDefault GpLvdsBias;
        //99
        Itkpixv2RegDefault GpCmosRoute;
        //100
        Itkpixv2RegDefault GpLvdsPad3;
        Itkpixv2RegDefault GpLvdsPad2;
        //101
        Itkpixv2RegDefault GpLvdsPad1;
        Itkpixv2RegDefault GpLvdsPad0;
        //102
        Itkpixv2RegDefault CdrCp;
        //103
        Itkpixv2RegDefault CdrCpFd;
        //104
        Itkpixv2RegDefault CdrCpBuff;
        //105
        Itkpixv2RegDefault CdrVco;
        //106
        Itkpixv2RegDefault CdrVcoBuff;
        //107
        Itkpixv2RegDefault SerSelOut3;
        Itkpixv2RegDefault SerSelOut2;
        Itkpixv2RegDefault SerSelOut1;
        Itkpixv2RegDefault SerSelOut0;
        //108
        Itkpixv2RegDefault SerInvTap;
        Itkpixv2RegDefault SerEnTap;
        Itkpixv2RegDefault SerEnLane;
        //109
        Itkpixv2RegDefault CmlBias2;
        //110
        Itkpixv2RegDefault CmlBias1;
        //111
        Itkpixv2RegDefault CmlBias0;
        //112
        Itkpixv2RegDefault MonitorEnable;
        Itkpixv2RegDefault MonitorI;
        Itkpixv2RegDefault MonitorV;
        //113
        Itkpixv2RegDefault ErrWngMask;
        //114
        Itkpixv2RegDefault MonSensSldoDigEn;
        Itkpixv2RegDefault MonSensSldoDigDem;
        Itkpixv2RegDefault MonSensSldoDigSelBias;
        Itkpixv2RegDefault MonSensSldoAnaEn;
        Itkpixv2RegDefault MonSensSldoAnaDem;
        Itkpixv2RegDefault MonSensSldoAnaSelBias;
        //115
        Itkpixv2RegDefault MonSensAcbEn;
        Itkpixv2RegDefault MonSensAcbDem;
        Itkpixv2RegDefault MonSensAcbSelBias;
        //116
        Itkpixv2RegDefault VrefRsensBot;
        Itkpixv2RegDefault VrefRsensTop;
        Itkpixv2RegDefault VrefIn;
        Itkpixv2RegDefault MonAdcTrim;
        //117
        Itkpixv2RegDefault NtcDac;
        //118-121
        Itkpixv2RegDefault HitOrMask3;
        Itkpixv2RegDefault HitOrMask2;
        Itkpixv2RegDefault HitOrMask1;
        Itkpixv2RegDefault HitOrMask0;
        //122-129
        Itkpixv2RegDefault AutoRead0;
        Itkpixv2RegDefault AutoRead1;
        Itkpixv2RegDefault AutoRead2;
        Itkpixv2RegDefault AutoRead3;
        Itkpixv2RegDefault AutoRead4;
        Itkpixv2RegDefault AutoRead5;
        Itkpixv2RegDefault AutoRead6;
        Itkpixv2RegDefault AutoRead7;
        //130
        Itkpixv2RegDefault RingOscBClear;
        Itkpixv2RegDefault RingOscBEnBl;
        Itkpixv2RegDefault RingOscBEnBr;
        Itkpixv2RegDefault RingOscBEnCapA;
        Itkpixv2RegDefault RingOscBEnFf;
        Itkpixv2RegDefault RingOscBEnLvt;
        Itkpixv2RegDefault RingOscAClear;
        Itkpixv2RegDefault RingOscAEn;
        //131
        Itkpixv2RegDefault RingOscARoute;
        Itkpixv2RegDefault RingOscBRoute;
        //133-133
        Itkpixv2RegDefault RingOscAOut;
        Itkpixv2RegDefault RingOscBOut;
        //134-136 RO
        Itkpixv2RegDefault BcidCnt;
        Itkpixv2RegDefault TrigCnt;
        Itkpixv2RegDefault ReadTrigCnt;
        //137-141
        Itkpixv2RegDefault LockLossCnt;
        Itkpixv2RegDefault BitFlipWngCnt;
        Itkpixv2RegDefault BitFlipErrCnt;
        Itkpixv2RegDefault CmdErrCnt;
        Itkpixv2RegDefault RdWrFifoErrCnt;
        //142
        Itkpixv2RegDefault AiRegionRow;
        //143-146
        Itkpixv2RegDefault HitOrCnt3;
        Itkpixv2RegDefault HitOrCnt2;
        Itkpixv2RegDefault HitOrCnt1;
        Itkpixv2RegDefault HitOrCnt0;
        //147-148
        Itkpixv2RegDefault GatedHitOrCnt1;
        Itkpixv2RegDefault GatedHitOrCnt0;
        //149-151
        Itkpixv2RegDefault PixelSeuCnt;
        Itkpixv2RegDefault GlobalConfigSeuCnt;
        Itkpixv2RegDefault SkippedTrigCnt;
        //152
        Itkpixv2RegDefault DataDecodingValues;
        //153-154
        Itkpixv2RegDefault EfuseReadData0;
        Itkpixv2RegDefault EfuseReadData1;
        //155
        Itkpixv2RegDefault MonitoringDataAdc;
        //156
        Itkpixv2RegDefault ChipIdSense;
        Itkpixv2RegDefault IrefTrimSense;
        //138-201
        //SEU_notmr - not implemented, do not need to store
        //202-255
        //SEU - not implemented, do not need to store
        
        // Special regs
        Itkpixv2DiffReg InjVcalDiff;
        Itkpixv2MultiReg DiffTh1;
};


#endif
