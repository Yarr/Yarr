#ifndef FE65P2PIXELCFG_H
#define FE65P2PIXELCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE65-p2 cpp Library
// # Comment: FE65-p2 Pixel Register container
// ################################

#include <stdint.h>
#include <array>

#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class QuadColumnBit {
    public:
        const static unsigned n_Words = 16;
    protected:
        std::array<uint16_t, n_Words> storage;
    public:
        void set(const uint16_t *bitstream);
        void setAll(const uint16_t val);
        void setPixel(const unsigned n, const uint16_t val);

        uint16_t* getStream();
        uint16_t getPixel(const unsigned n);
        uint16_t getWord(const unsigned n);
};

template<unsigned N, bool msbRight>
class QuadColumnField {
    protected:
        QuadColumnBit field[N];
    public:
        QuadColumnBit& operator[](const unsigned bit) {
            return field[bit];
        }

        void set(QuadColumnBit *bitstreams) {
            for (unsigned i=0; i<N; i++)
                field[i] = bitstreams[i];
        }

        void setAll(const uint16_t val) {
            for(unsigned i=0; i<N; i++) {
                uint16_t bit = (val>>i)&0x1;
                if (msbRight) {
                        field[(N-i-1)].setAll(bit);
                } else {   
                        field[i].setAll(bit);
                }
            }
        }

        
        void setPixel(const unsigned n, uint16_t val) {
            for(unsigned i=0; i<N; i++) {
                uint16_t bit = (val>>i)&0x1;
                if (msbRight) {
                        field[(N-i-1)].setPixel(n, bit);
                } else {   
                        field[i].setPixel(n, bit);
                }
            }
        }

        uint16_t getPixel(const unsigned n) {
            uint16_t tmp = 0;
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


class Fe65p2PixelCfg {
    public:
        const static unsigned n_QC = 16;
        const static unsigned n_Bits = 8;
        const static unsigned n_Words = 16;
        const static unsigned n_Col = 64;
        const static unsigned n_Row = 64;
    private:
        QuadColumnBit m_Sign[n_QC];
        QuadColumnBit m_InjEn[n_QC];
        QuadColumnField<4, false> m_TDAC[n_QC];
        QuadColumnField<2, false> m_PixConf[n_QC];
    public:
        Fe65p2PixelCfg();
        uint16_t* getCfg(unsigned bit, unsigned qc);

        QuadColumnBit& Sign(unsigned qc);
        QuadColumnBit& InjEn(unsigned qc);
        QuadColumnField<4, false>& TDAC(unsigned qc);
        QuadColumnField<2, false>& PixConf(unsigned qc);

        void setSign(unsigned col, unsigned row, unsigned v);
        void setInjEn(unsigned col, unsigned row, unsigned v);
        void setTDAC(unsigned col, unsigned row, unsigned v);
        void setPixConf(unsigned col, unsigned row, unsigned v);
        unsigned getSign(unsigned col, unsigned row); 
        unsigned getInjEn(unsigned col, unsigned row); 
        unsigned getTDAC(unsigned col, unsigned row); 
        unsigned getPixConf(unsigned col, unsigned row); 

        static unsigned to_qc(unsigned col);
        static unsigned to_bit(unsigned col, unsigned row);
    protected:
        void toFileJson(json &j);
        void fromFileJson(json &j);

};

#endif
