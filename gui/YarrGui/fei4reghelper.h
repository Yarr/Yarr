#ifndef FEI4REGHELPER_H
#define FEI4REGHELPER_H

#include <iomanip>
#include <iostream>
#include <stdint.h>

#include <QMetaType>

#include "Fei4.h"
#include "Fei4GlobalFeedback.h"
#include "LoopActionBase.h"

class Fei4RegHelper {
public:
    Fei4RegHelper() : mOffset(0), bOffset(0), mask(0), msbRight(false) {} //Necessary for Q_DECLARE_METATYPE
    Fei4RegHelper(unsigned int p1, unsigned int p2, unsigned int p3, bool p4)
        : mOffset(p1), bOffset(p2), mask(p3), msbRight(p4) {}
    Fei4RegHelper(Fei4RegHelper const& other)                             //Necessary for Q_DECLARE_METATYPE
        : mOffset(other.getMOffset()), bOffset(other.getBOffset()),
          mask(other.getMask()), msbRight(other.getMsbRight()) {}
    ~Fei4RegHelper() {}

    unsigned int getMOffset() const;
    unsigned int getBOffset() const;
    unsigned int getMask() const;
    bool getMsbRight() const;

    void printReg() const;

    void writeReg(Fei4 * fe, uint16_t v);

    bool operator==(Fei4RegHelper const& other) const;

protected:
    unsigned int mOffset;     //Number of the 16 bit bar (there are 35)
    unsigned int bOffset;     //Starting bit of register (within 16 bit bar)
    unsigned int mask;        //number of bits of GR (within 16 bit bar)
    bool msbRight;
    //Exactly 16 bits necessary - maybe pack those 4 together in one bit field or so?
};

//###################################################################################

class Fei4PLHelper : public Fei4RegHelper, public LoopActionBase {
public:
    Fei4PLHelper() : cur(0) {}
    Fei4PLHelper(Fei4RegHelper const& f) : Fei4RegHelper(f), cur(0) {}
    Fei4PLHelper(unsigned int p1, unsigned int p2, unsigned int p3, bool p4)
        : Fei4RegHelper(p1, p2, p3, p4), cur(0) {}
    Fei4PLHelper(Fei4PLHelper const& other)
        : Fei4RegHelper(other.getMOffset(), other.getBOffset(), other.getMask(), other.getMsbRight()), cur(0) {}
    ~Fei4PLHelper() {}

    void init();
    void execPart1();
    void execPart2();
    void end();

private:
    uint16_t cur;
};

//##################################################################################

class Fei4GFHelper : public Fei4RegHelper, public Fei4GlobalFeedbackBase {
public:
    Fei4GFHelper() : Fei4RegHelper() {}
    Fei4GFHelper(unsigned int p1, unsigned int p2, unsigned int p3, bool p4) : Fei4RegHelper(p1, p2, p3, p4) {}
    Fei4GFHelper(Fei4RegHelper const& f) : Fei4RegHelper(f) {}
    Fei4GFHelper(Fei4GFHelper const& other)
        : Fei4RegHelper(other.getMOffset(), other.getBOffset(), other.getMask(), other.getMsbRight()) {}
    ~Fei4GFHelper() {}

private:
    void init();
    void execPart1();
    void execPart2();
    void end();
    void writePar();
    bool allDone();
};

Q_DECLARE_METATYPE(Fei4RegHelper)

#endif // FEI4REGHELPER_H
