#ifndef FE65P2GLOBALCFG_H
#define FE65P2GLOBALCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE65-p2 cpp Library
// # Comment: FE65-p2 Global Register container
// ################################

#include <iostream>
#include <stdint.h>
#include <array>
#include <map>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Fe65p2GlobalReg {
    private:
        uint16_t *m_cfg;
        unsigned m_offset;
    public:
        Fe65p2GlobalReg(){}
        
        void initReg(uint16_t *cfg, const unsigned offset, const uint16_t cfgBits) {
            m_cfg = cfg;
            m_offset = offset;
            this->write(cfgBits);
        }

        void write(const uint16_t cfgBits) {
            m_cfg[m_offset] = cfgBits;
        }

        uint16_t read() const{
            return m_cfg[m_offset];
        }

        unsigned addr() const {
            return m_offset;
        }
};

class Fe65p2GlobalCfg {
    public:
        Fe65p2GlobalCfg();
        ~Fe65p2GlobalCfg();

        static const unsigned numRegs = 23;

        void setValue(Fe65p2GlobalReg Fe65p2GlobalCfg::*reg, const uint16_t cfgBits) {
            (this->*reg).write(cfgBits);
        }

        uint16_t getValue(Fe65p2GlobalReg Fe65p2GlobalCfg::*reg) {
            return (this->*reg).read();
        }

        unsigned getAddr(Fe65p2GlobalReg Fe65p2GlobalCfg::*reg) {
            return (this->*reg).addr();
        }

        std::map<std::string, Fe65p2GlobalReg*> regMap;

        // Pixel Shadow register
        Fe65p2GlobalReg TestHit;
        Fe65p2GlobalReg SignLd;
        Fe65p2GlobalReg InjEnLd;
        Fe65p2GlobalReg TDacLd;
        Fe65p2GlobalReg PixConfLd;
        Fe65p2GlobalReg SPARE_0;

        // Global config
        Fe65p2GlobalReg OneSr;
        Fe65p2GlobalReg HitDisc;
        Fe65p2GlobalReg Latency;
        Fe65p2GlobalReg ColEn;
        Fe65p2GlobalReg ColSrEn;
        Fe65p2GlobalReg ColSrOut;
        Fe65p2GlobalReg SPARE_1;
        Fe65p2GlobalReg PrmpVbpDac;
        Fe65p2GlobalReg Vthin1Dac;
        Fe65p2GlobalReg Vthin2Dac;
        Fe65p2GlobalReg VffDac;
        Fe65p2GlobalReg VctrCF0Dac;
        Fe65p2GlobalReg VctrCF1Dac;
        Fe65p2GlobalReg PrmpVbnFolDac;
        Fe65p2GlobalReg VbnLccDac;
        Fe65p2GlobalReg CompVbnDac;
        Fe65p2GlobalReg PreCompVbnDac;

        // Not actually in Fe65p2, for sw reason here
        Fe65p2GlobalReg PlsrDac;
        Fe65p2GlobalReg PlsrDelay;
        Fe65p2GlobalReg TrigCount;

    protected:
        void toFileJson(json &j);
        void fromFileJson(json &j);
        
        void init();
        uint16_t cfg[numRegs];
        uint16_t plsrDacReg;
        uint16_t plsrDelayReg;
        uint16_t trigCountReg;
    private:
};

#endif

