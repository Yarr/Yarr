#ifndef RD53APIXELCFG_H
#define RD53APIXELCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A pixel config
// # Date: Jan 2018
// ################################

#include <iostream>
#include <array>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Rd53aPixelCfg {
    public:
        static const unsigned n_DC= 200;
        static const unsigned n_Col = 400;
        static const unsigned n_Row = 192;
    private:
        std::array<uint16_t, n_DC*n_Row> pixRegs;

        inline uint16_t maskBits(uint16_t val, unsigned mask);
    public:
        Rd53aPixelCfg();

        void setEn(unsigned col, unsigned row, unsigned v);
        void setHitbus(unsigned col, unsigned row, unsigned v);
        void setInjEn(unsigned col, unsigned row, unsigned v);
        void setTDAC(unsigned col, unsigned row, unsigned v);

        unsigned getEn(unsigned col, unsigned row);
        unsigned getHitbus(unsigned col, unsigned row);
        unsigned getInjEn(unsigned col, unsigned row);
        unsigned getTDAC(unsigned col, unsigned row);

        inline unsigned toIndex(unsigned col, unsigned row);
    protected:
        void toFileJson(json &j);
        void fromFileJson(json &j);

};

#endif
