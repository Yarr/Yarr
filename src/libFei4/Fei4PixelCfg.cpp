// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 cpp Library
// # Comment: FE-I4 Pixel Register container
// ################################

#include "Fei4PixelCfg.h"

void DoubleColumnBit::set(const uint32_t *bitstream) {
    for(unsigned i=0; i<n_Words; i++)
        storage[i] = bitstream[i];
}

void DoubleColumnBit::setAll(const uint32_t val) {
    for(unsigned i=0; i<n_Words; i++) {
        storage[i] = 0;
        for (unsigned j=0; j<32; j++) {
            uint32_t tmp = val << j;
            storage[i] |= tmp;
        }
    }
}

void DoubleColumnBit::setPixel(const unsigned n, uint32_t val) {
    val <<= (n)%32;
    uint32_t mask = 0x1 << ((n)%32);
    storage[(n)/32] = val | (storage[(n)/32]&(~mask));
}

uint32_t* DoubleColumnBit::getStream() {
    return &storage[0];
}

uint32_t DoubleColumnBit::getPixel(const unsigned n) {
    uint32_t mask = 0x1 << ((n)%32);
    return ((storage[(n)/32] & mask) >> ((n)%32));
}

uint32_t DoubleColumnBit::getWord(const unsigned n) {
    return storage[n];
}

Fei4PixelCfg::Fei4PixelCfg() {
    for (unsigned i=0; i<n_DC; i++) {
        m_En[i].setAll(1);
        m_TDAC[i].setAll(16);
        m_LCap[i].setAll(0);
        m_SCap[i].setAll(0);
        m_Hitbus[i].setAll(0);
        m_FDAC[i].setAll(8);
    }
}

uint32_t* Fei4PixelCfg::getCfg(unsigned bit, unsigned dc) {
    switch (bit) {
        case 0:
            return m_En[dc].getStream();
            break;
        case 1:
            return m_TDAC[dc][0].getStream();
            break;
        case 2:
            return m_TDAC[dc][1].getStream();
            break;
        case 3:
            return m_TDAC[dc][2].getStream();
            break;
        case 4:
            return m_TDAC[dc][3].getStream();
            break;
        case 5:
            return m_TDAC[dc][4].getStream();
            break;
        case 6:
            return m_LCap[dc].getStream();
            break;
        case 7:
            return m_SCap[dc].getStream();
            break;
        case 8:
            return m_Hitbus[dc].getStream();
            break;
        case 9:
            return m_FDAC[dc][0].getStream();
            break;
        case 10:
            return m_FDAC[dc][1].getStream();
            break;
        case 11:
            return m_FDAC[dc][2].getStream();
            break;
        case 12:
            return m_FDAC[dc][3].getStream();
            break;
        default:
            return NULL;
            break;
    }
}

DoubleColumnBit& Fei4PixelCfg::En(unsigned dc) {
    return m_En[dc];
}

DoubleColumnField<5, true>& Fei4PixelCfg::TDAC(unsigned dc) {
    return m_TDAC[dc];
}

DoubleColumnBit& Fei4PixelCfg::LCap(unsigned dc) {
    return m_LCap[dc];
}


DoubleColumnBit& Fei4PixelCfg::SCap(unsigned dc) {
    return m_SCap[dc];
}

DoubleColumnBit& Fei4PixelCfg::Hitbus(unsigned dc) {
    return m_Hitbus[dc];
}

DoubleColumnField<4, false>& Fei4PixelCfg::FDAC(unsigned dc) {
    return m_FDAC[dc];
}

unsigned Fei4PixelCfg::to_dc(unsigned col) {
    return ((col-1)/2);
}

unsigned Fei4PixelCfg::to_bit(unsigned col, unsigned row) {
    unsigned bit = 0;
    // Bit 671 = 2,336
    // Bit 336 = 2,1
    // Bit 335 = 1,1
    // Bit 0   = 1,336
    if (col%2 == 0) {
        bit = row+335;
    } else {
        bit = 336-row;
    }
    return bit;
}

void Fei4PixelCfg::setEn(unsigned col, unsigned row, unsigned v) {
    m_En[to_dc(col)].setPixel(to_bit(col, row) , v);
}

void Fei4PixelCfg::setHitbus(unsigned col, unsigned row, unsigned v) {
    m_Hitbus[to_dc(col)].setPixel(to_bit(col, row) , v);
}

void Fei4PixelCfg::setTDAC(unsigned col, unsigned row, unsigned v) {
    m_TDAC[to_dc(col)].setPixel(to_bit(col, row) , v);
}

void Fei4PixelCfg::setLCap(unsigned col, unsigned row, unsigned v) {
    m_LCap[to_dc(col)].setPixel(to_bit(col, row) , v);
}

void Fei4PixelCfg::setSCap(unsigned col, unsigned row, unsigned v) {
    m_SCap[to_dc(col)].setPixel(to_bit(col, row) , v);
}

void Fei4PixelCfg::setFDAC(unsigned col, unsigned row, unsigned v) {
    m_FDAC[to_dc(col)].setPixel(to_bit(col, row) , v);
}

unsigned Fei4PixelCfg::getEn(unsigned col, unsigned row) {
    return m_En[to_dc(col)].getPixel(to_bit(col, row));
}

unsigned Fei4PixelCfg::getHitbus(unsigned col, unsigned row) {
    return m_Hitbus[to_dc(col)].getPixel(to_bit(col, row));
}

unsigned Fei4PixelCfg::getTDAC(unsigned col, unsigned row) {
    return m_TDAC[to_dc(col)].getPixel(to_bit(col, row));
}

unsigned Fei4PixelCfg::getLCap(unsigned col, unsigned row) {
    return m_LCap[to_dc(col)].getPixel(to_bit(col, row));
}

unsigned Fei4PixelCfg::getSCap(unsigned col, unsigned row) {
    return m_SCap[to_dc(col)].getPixel(to_bit(col, row));
}

unsigned Fei4PixelCfg::getFDAC(unsigned col, unsigned row) {
    return m_FDAC[to_dc(col)].getPixel(to_bit(col, row));
}

void Fei4PixelCfg::toFileXml(tinyxml2::XMLDocument *doc, tinyxml2::XMLElement *node) {
    tinyxml2::XMLElement *pcfg = doc->NewElement("PixelConfig");
    
    tinyxml2::XMLElement *reg = NULL;
    for (unsigned col=1; col<=80; col++) {
        for (unsigned row=1; row<=336; row++) {
            reg = doc->NewElement("Pixel");
            reg->SetAttribute("Col", col);
            reg->SetAttribute("Row", row);
            reg->SetAttribute("Enable", getEn(col, row));
            reg->SetAttribute("Hitbus", getHitbus(col, row));
            reg->SetAttribute("TDAC", getTDAC(col, row));
            reg->SetAttribute("FDAC", getFDAC(col, row));
            pcfg->LinkEndChild(reg);
        }
    }


    node->LinkEndChild(pcfg);
}

void Fei4PixelCfg::toFileJson(json &j) {
    // Layout is one array per column
    for (unsigned row=1; row<=n_Row; row++) {
        for (unsigned col=1; col<=n_Col; col++) {
            j["FE-I4B"]["PixelConfig"][row-1]["Row"] = row;
            j["FE-I4B"]["PixelConfig"][row-1]["Enable"][col-1] = getEn(col, row);
            j["FE-I4B"]["PixelConfig"][row-1]["Hitbus"][col-1] = getHitbus(col, row);
            j["FE-I4B"]["PixelConfig"][row-1]["TDAC"][col-1] = getTDAC(col, row);
            j["FE-I4B"]["PixelConfig"][row-1]["LCap"][col-1] = getLCap(col, row);
            j["FE-I4B"]["PixelConfig"][row-1]["SCap"][col-1] = getSCap(col, row);
            j["FE-I4B"]["PixelConfig"][row-1]["FDAC"][col-1] = getFDAC(col, row);
        }
    }

    /*
    // Not human readble pixel regs
    for (unsigned dc=0; dc<n_DC; dc++) {
            j["FE-I4B"]["PixelConfig"]["Enable"][dc] = m_En[dc].getArray();
            j["FE-I4B"]["PixelConfig"]["Hitbus"][dc] = m_Hitbus[dc].getArray();
            for (unsigned bit=0; bit<5; bit++)
                j["FE-I4B"]["PixelConfig"]["TDAC"][dc][bit] = m_TDAC[dc][bit].getArray();
            for (unsigned bit=0; bit<5; bit++)
                j["FE-I4B"]["PixelConfig"]["FDAC"][dc][bit] = m_FDAC[dc][bit].getArray();
    }*/
}

void Fei4PixelCfg::fromFileJson(json &j) {
    // Layout is one array per column
    if (j["FE-I4B"]["PixelConfig"].empty())
        return;
    for (unsigned row=1; row<=n_Row; row++) {
        for (unsigned col=1; col<=n_Col; col++) {
            setEn(col, row, j["FE-I4B"]["PixelConfig"][row-1]["Enable"][col-1]);
            setHitbus(col, row, j["FE-I4B"]["PixelConfig"][row-1]["Hitbus"][col-1]);
            setTDAC(col, row, j["FE-I4B"]["PixelConfig"][row-1]["TDAC"][col-1]);
            setLCap(col, row, j["FE-I4B"]["PixelConfig"][row-1]["LCap"][col-1]);
            setSCap(col, row, j["FE-I4B"]["PixelConfig"][row-1]["SCap"][col-1]);
            setFDAC(col, row, j["FE-I4B"]["PixelConfig"][row-1]["FDAC"][col-1]);
        }
    }

}
