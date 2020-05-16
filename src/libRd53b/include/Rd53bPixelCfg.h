#ifndef RD53BPIXELCFG_H
#define RD53BPIXELCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B Pixel config class
// # Date: May 2020
// ################################

#include "storage.hpp"

class Rd53bPixelCfg {
    public:
        Rd53bPixelCfg();
        //~Rd53bPixelCfg();
        
        void setEn(unsigned col, unsigned row, unsigned v);
        void setHitbus(unsigned col, unsigned row, unsigned v);
        void setInjEn(unsigned col, unsigned row, unsigned v);
        void setTDAC(unsigned col, unsigned row, int v);

        unsigned getEn(unsigned col, unsigned row);
        unsigned getHitbus(unsigned col, unsigned row);
        unsigned getInjEn(unsigned col, unsigned row);
        int getTDAC(unsigned col, unsigned row);

        static constexpr unsigned n_DC = 200;
        static constexpr unsigned n_Col = 400;
        static constexpr unsigned n_Row = 384;
    
    protected:
        std::array<std::array<uint16_t, n_Row>, n_DC> pixRegs;

        void toJson(json &j);
        void fromJson(json &j);
    
    private:

        struct pixelFields {
            unsigned en : 1;
            unsigned injen : 1;
            unsigned hitbus : 1;
            unsigned tdac : 4;
            unsigned sign : 1;
        };

        union pixelBits {
            Rd53bPixelCfg::pixelFields s;
            uint8_t u8;
        };
};

#endif
