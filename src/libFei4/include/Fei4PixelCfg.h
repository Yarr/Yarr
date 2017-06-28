#ifndef FEI4PIXELCFG_H
#define FEI4PIXELCFG_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 cpp Library
// # Comment: FE-I4 Pixel Register container
// ################################

#include <stdint.h>
#include <iostream>
#include <array>

#include "tinyxml2.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class DoubleColumnBitOps {
	public:
		virtual void set(const uint32_t *bitstream) = 0;
		virtual void setAll(const uint32_t val) = 0;
		virtual void setPixel(const unsigned n, uint32_t val) = 0;
		virtual uint32_t* getStream() = 0;
		virtual uint32_t getPixel(const unsigned n) = 0;
		virtual uint32_t getWord(const unsigned n) = 0;
};

class DoubleColumnBit: public DoubleColumnBitOps {
    public:
        const static unsigned n_Words = 21;
    protected:
        std::array<uint32_t, n_Words> storage;
    public:    

        void set(const uint32_t *bitstream);
        void setAll(const uint32_t val);
        void setPixel(const unsigned n, uint32_t val);

        uint32_t* getStream();
        std::array<uint32_t, n_Words> getArray() {return storage;}
        uint32_t getPixel(const unsigned n);
        uint32_t getWord(const unsigned n);
};

template <unsigned N, bool msbRight>
class DoubleColumnField {
    protected:
        DoubleColumnBit field[N];
    public:
        DoubleColumnBit& operator[](const unsigned bit) {
            return field[bit];
        }

        void set(DoubleColumnBit *bitstreams) {
            for (unsigned i=0; i<N; i++)
                field[i] = bitstreams[i];
        }

        void setAll(const uint32_t val) {
            for(unsigned i=0; i<N; i++) {
                uint32_t bit = (val>>i)&0x1;
                if (msbRight) {
                        field[(N-i-1)].setAll(bit);
                } else {   
                        field[i].setAll(bit);
                }
            }
        }

        
        void setPixel(const unsigned n, uint32_t val) {
            for(unsigned i=0; i<N; i++) {
                uint32_t bit = (val>>i)&0x1;
                if (msbRight) {
                        field[(N-i-1)].setPixel(n, bit);
                } else {   
                        field[i].setPixel(n, bit);
                }
            }
        }

        uint32_t getPixel(const unsigned n) {
            uint32_t tmp = 0;
            for(unsigned i=0; i<N; i++) {
                if (msbRight) {
                    tmp |= (field[(N-i-1)].getPixel(n))<<i;
                } else {
                    tmp |= (field[i].getPixel(n))<<i;
                }
            }
            return tmp;
        }
};
            
class Fei4PixelCfg {
    public:
        const static unsigned n_DC = 40;
        const static unsigned n_Col = 80;
        const static unsigned n_Row = 336;
        const static unsigned n_Bits = 13;
        const static unsigned n_Words = 21;
    private:
        // Config represantation
        DoubleColumnBit m_En[n_DC];
        DoubleColumnField<5, true> m_TDAC[n_DC];
        DoubleColumnBit m_LCap[n_DC];
        DoubleColumnBit m_SCap[n_DC];
        DoubleColumnBit m_Hitbus[n_DC];
        DoubleColumnField<4, false> m_FDAC[n_DC];
    public:
        Fei4PixelCfg();

        uint32_t* getCfg(unsigned bit, unsigned dc);

        DoubleColumnBit& En(unsigned dc);
        DoubleColumnField<5, true>& TDAC(unsigned dc);
        DoubleColumnBit& LCap(unsigned dc);
        DoubleColumnBit& SCap(unsigned dc);
        DoubleColumnBit& Hitbus(unsigned dc);
        DoubleColumnField<4, false>& FDAC(unsigned dc);

        void setEn(unsigned col, unsigned row, unsigned v);
        void setHitbus(unsigned col, unsigned row, unsigned v);
        void setTDAC(unsigned col, unsigned row, unsigned v);
        void setLCap(unsigned col, unsigned row, unsigned v);
        void setSCap(unsigned col, unsigned row, unsigned v);
        void setFDAC(unsigned col, unsigned row, unsigned v);
        unsigned getEn(unsigned col, unsigned row);
        unsigned getHitbus(unsigned col, unsigned row);
        unsigned getTDAC(unsigned col, unsigned row);
        unsigned getLCap(unsigned col, unsigned row);
        unsigned getSCap(unsigned col, unsigned row);
        unsigned getFDAC(unsigned col, unsigned row);

        static unsigned to_dc(unsigned col);
        static unsigned to_bit(unsigned col, unsigned row);

        void toFileXml(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *node);
        
        void toFileJson(json &j);
        void fromFileJson(json &j);
};

#endif
