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
        Rd53aReg();
    
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
};
#endif

