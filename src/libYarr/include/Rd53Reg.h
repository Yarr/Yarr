//
// Created by wittgen on 10/30/22.
//

#ifndef YARR_RD53REG_H
#define YARR_RD53REG_H

#include <cinttypes>
class Rd53Reg {
public:
    Rd53Reg() = default;
    virtual ~Rd53Reg() = default;
    virtual void write(uint16_t value) = 0;
    virtual uint16_t read() const = 0;


    uint16_t applyMask(uint16_t value) const {
        unsigned mask = (1<<m_bits)-1;
        return ((value >> m_bOffset) & mask);
    }

    unsigned addr() const{
        return m_addr;
    }

    unsigned bits() const {
        return m_bits;
    }
protected:
    uint16_t *m_cfg = nullptr;
    unsigned m_bOffset = 0;
    unsigned m_bits = 9;
    unsigned m_addr = 999;
};

#endif //YARR_RD53REG_H
