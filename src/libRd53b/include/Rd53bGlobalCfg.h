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

#include <map>
#include <string>

#include "storage.hpp"

class Rd53bReg {
    public:
        Rd53bReg() {
            m_cfg = NULL;
            m_bOffset = 0;
            m_bits = 0;
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

class Rd53bGlobalCfg {
    public:
        Rd53bGlobalCfg();
        ~Rd53bGlobalCfg();

        void init();

        uint16_t getValue(Rd53bReg Rd53bGlobalCfg::*ref);
        uint16_t getValue(std::string name);
        void setValue(Rd53bReg Rd53bGlobalCfg::*ref, uint16_t val);
        void setValue(std::string name, uint16_t val);

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
        Rd53bReg PixMode;
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
        Rd53bReg SldoEnUndershuntB;
        Rd53bReg SldoTrimA;
        Rd53bReg SldoTrimB;

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
        Rd53bReg TrigMode;
        Rd53bReg Latency;
        //48
        Rd53bReg SelfTrigEn;
        Rd53bReg SelfTrigDigThrEn;
        Rd53bReg SelfTrigDigThr;
        //49
        Rd53bReg SelTrigDelay;
        Rd53bReg SelfTrigMulti;
        //50
        Rd53bReg SelfTrigPattern;
        //51
        Rd53bReg ColReadDelay;
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
        Rd53bReg DataMergeEnClk;
        Rd53bReg DataMergeEn;
        Rd53bReg DataMergeEnBond;

};


#endif
