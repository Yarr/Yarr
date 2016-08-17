#include "Fe65p2PixelCfg.h"

void QuadColumnBit::set(const uint16_t *bitstream) {
    for(unsigned i=0; i<n_Words; i++) {
        storage[i] = bitstream[i];
    }
}

void QuadColumnBit::setAll(const uint16_t val) {
    for(unsigned i=0; i<n_Words; i++) {
        storage[i] = 0;
        for (unsigned j=0; j<16; j++) {
            uint16_t tmp = val << j;
            storage[i] |= tmp;
        }
    }
}

void QuadColumnBit::setPixel(const unsigned n, uint16_t val) {
    val <<= (n)%16;
    uint16_t mask = 0x1 << ((n)%16);
    storage[(n)/16] = val | (storage[(n)/16]&(~mask));
}

uint16_t* QuadColumnBit::getStream() {
    return &storage[0];
}

uint16_t QuadColumnBit::getPixel(const unsigned n) {
    uint16_t mask = 0x1 << ((n)%16);
    return ((storage[(n)/16] & mask) >> ((n)%16));
}

uint16_t QuadColumnBit::getWord(const unsigned n) {
    return storage[n];
}

Fe65p2PixelCfg::Fe65p2PixelCfg() {
    for (unsigned i=0; i<n_QC; i++) {
        m_Sign[i].setAll(0);
        m_InjEn[i].setAll(0);
        m_TDAC[i].setAll(0);
        m_PixConf[i].setAll(3);
    }
}

uint16_t* Fe65p2PixelCfg::getCfg(unsigned bit, unsigned qc) {
    switch (bit) {
        case 0:
            return m_Sign[qc].getStream();
            break;
        case 1:
            return m_InjEn[qc].getStream();
            break;
        case 2:
            return m_TDAC[qc][0].getStream();
            break;
        case 3:
            return m_TDAC[qc][1].getStream();
            break;
        case 4:
            return m_TDAC[qc][2].getStream();
            break;
        case 5:
            return m_TDAC[qc][3].getStream();
            break;
        case 6:
            return m_PixConf[qc][0].getStream();
            break;
        case 7:
            return m_PixConf[qc][1].getStream();
            break;
        default:
            return NULL;
            break;
    }
}


unsigned Fe65p2PixelCfg::to_qc(unsigned col) { //1-64
    return (col-1)/4;
}

unsigned Fe65p2PixelCfg::to_bit(unsigned col, unsigned row) {
    // SR indexing per QC:
    // 124 126 128 130
    // 125 127 129 131
    // ... ... ... ...
    //  5   7  249 251
    //  0   2  252 254
    //  1   3  253 255

    switch ((col-1)%4) {
        case 0:
            return (4*((row-1)/2))+(1-((row-1)%2));
            break;
        case 1:
            return 2+(4*((row-1)/2))+(1-((row-1)%2));
            break;
        case 2:
            return 253-(4*((row-1)/2))-((row-1)%2);
            break;
        case 3:
            return 255-(4*((row-1)/2))-((row-1)%2);
            break;
        default:
            return 0;
            break;
    }
    return 0;
}

QuadColumnBit& Fe65p2PixelCfg::Sign(unsigned qc) {
    return m_Sign[qc];
}

QuadColumnBit& Fe65p2PixelCfg::InjEn(unsigned qc) {
    return m_InjEn[qc];
}

QuadColumnField<4, false>& Fe65p2PixelCfg::TDAC(unsigned qc) {
    return m_TDAC[qc];
}

QuadColumnField<2, false>& Fe65p2PixelCfg::PixConf(unsigned qc) {
    return m_PixConf[qc];
}

void Fe65p2PixelCfg::setSign(unsigned col, unsigned row, unsigned v) {
    m_Sign[to_qc(col)].setPixel(to_bit(col, row), v);
}


void Fe65p2PixelCfg::setInjEn(unsigned col, unsigned row, unsigned v) {
    m_InjEn[to_qc(col)].setPixel(to_bit(col, row), v);
}

void Fe65p2PixelCfg::setTDAC(unsigned col, unsigned row, unsigned v) {
    m_TDAC[to_qc(col)].setPixel(to_bit(col, row), v);
}

void Fe65p2PixelCfg::setPixConf(unsigned col, unsigned row, unsigned v) {
    m_PixConf[to_qc(col)].setPixel(to_bit(col, row), v);
}

unsigned Fe65p2PixelCfg::getSign(unsigned col, unsigned row) {
    return m_Sign[to_qc(col)].getPixel(to_bit(col, row));
}

unsigned Fe65p2PixelCfg::getInjEn(unsigned col, unsigned row) {
    return m_InjEn[to_qc(col)].getPixel(to_bit(col, row));
}

unsigned Fe65p2PixelCfg::getTDAC(unsigned col, unsigned row) {
    return m_TDAC[to_qc(col)].getPixel(to_bit(col, row));
}

unsigned Fe65p2PixelCfg::getPixConf(unsigned col, unsigned row) {
    return m_PixConf[to_qc(col)].getPixel(to_bit(col, row));
}

void Fe65p2PixelCfg::toFileJson(json &j) {
    for (unsigned col=1; col<=n_Col; col++) {
        for (unsigned row=1; row<=n_Row; row++) {
            j["FE65-P2"]["PixelConfig"][col-1]["Col"] = col;
            j["FE65-P2"]["PixelConfig"][col-1]["Sign"][row-1] = getSign(col, row);
            j["FE65-P2"]["PixelConfig"][col-1]["InjEn"][row-1] = getInjEn(col, row);
            j["FE65-P2"]["PixelConfig"][col-1]["TDAC"][row-1] = getTDAC(col, row);
            j["FE65-P2"]["PixelConfig"][col-1]["PixConf"][row-1] = getPixConf(col, row);
        }
    }
}

void Fe65p2PixelCfg::fromFileJson(json &j) {
    for (unsigned col=1; col<=n_Col; col++) {
        for (unsigned row=1; row<=n_Row; row++) {
            setSign(col, row, j["FE65-P2"]["PixelConfig"][col-1]["Sign"][row-1]);
            setInjEn(col, row, j["FE65-P2"]["PixelConfig"][col-1]["InjEn"][row-1]);
            setTDAC(col, row, j["FE65-P2"]["PixelConfig"][col-1]["TDAC"][row-1]);
            setPixConf(col, row, j["FE65-P2"]["PixelConfig"][col-1]["PixConf"][row-1]);
        }
    }

}
