#ifndef ITKPIXV2PIXELCFG_H
#define ITKPIXV2PIXELCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 Pixel config class
// # Date: Jul 2023
// ################################

#include <array>
#include <bitset>

#include "storage.hpp"

class Itkpixv2PixelCfg {
    public:
        Itkpixv2PixelCfg();
        //~Itkpixv2PixelCfg();
 
        void setReg(unsigned col, unsigned row, unsigned en, unsigned injen, unsigned hitbus, int tdac);
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
    
        struct pixelFields {
            unsigned en : 1;
            unsigned injen : 1;
            unsigned hitbus : 1;
            unsigned tdac : 4;
            unsigned sign : 1;
        };

        union pixelBits {
            Itkpixv2PixelCfg::pixelFields s;
            uint8_t u8;
        };
        
        using PixelArray = std::array<std::array<uint16_t, n_Row>, n_DC>;

        PixelArray pixRegs;
        static uint16_t getPixelBit(PixelArray &input, unsigned col, unsigned row, unsigned bit);
        static uint16_t toTenBitMask(uint16_t pixReg);

    protected:
        void writeConfig(json &j);
        void loadConfig(const json &j);
    
    private:
        inline static void setBit(uint16_t &in, uint8_t bit, uint8_t val);
        inline static uint16_t getBit(uint16_t in, uint8_t bit);

};

#endif
