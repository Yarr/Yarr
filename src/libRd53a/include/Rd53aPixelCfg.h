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



#include "storage.hpp"

class Rd53aPixelCfg {
    public:
        static constexpr unsigned n_DC= 200;
        static constexpr unsigned n_Col = 400;
        static constexpr unsigned n_Row = 192;
        std::array<uint16_t, n_DC*n_Row> pixRegs;
    private:

        inline uint16_t maskBits(uint16_t val, unsigned mask);
    public:
        Rd53aPixelCfg();

        void setEn(unsigned col, unsigned row, unsigned v);
        void setHitbus(unsigned col, unsigned row, unsigned v);
        void setInjEn(unsigned col, unsigned row, unsigned v);
        void setTDAC(unsigned col, unsigned row, int v);

        unsigned getEn(unsigned col, unsigned row);
        unsigned getHitbus(unsigned col, unsigned row);
        unsigned getInjEn(unsigned col, unsigned row);
        int getTDAC(unsigned col, unsigned row);

        inline static unsigned toIndex(unsigned col, unsigned row) {
            return (col/2)*n_Row+row;
        }

    protected:
        void toFileJson(json &j);
        void fromFileJson(json &j);

        struct pixelFields {
            unsigned en : 1;
            unsigned injen : 1;
            unsigned hitbus : 1;
            unsigned tdac : 4;
            unsigned sign : 1;
        };

        union pixelBits {
            Rd53aPixelCfg::pixelFields s;
            uint8_t u8;
        };
};

#endif
