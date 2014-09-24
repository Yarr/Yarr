#ifndef FEI4
#define FEI4

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Base class
// ################################

#include <iostream>

#include "TxCore.h"
#include "Fei4Cmd.h"
#include "Fei4GlobalCfg.h"

enum MASK_STAGE {
    MASK_1  = 0xFFFFFFFF,
    MASK_2  = 0x55555555,
    MASK_4  = 0x11111111,
    MASK_8  = 0x01010101,
    MASK_16 = 0x00010001,
    MASK_32 = 0x00000001,
    MASK_NONE    = 0x00000000
};

class Fei4 : public Fei4GlobalCfg, public Fei4Cmd {
    public:
        Fei4(TxCore *arg_core, unsigned channel, unsigned chipId);

        void sendConfig();

        void setRunMode(bool mode=true) {
            runMode(chipId, mode);
        }

        void initMask(enum MASK_STAGE mask);
        void shiftMask();
        void loadIntoShiftReg(unsigned pixel_latch);
        void loadIntoPixel(unsigned pixel_latch);
        void shiftByOne();

        template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
            void writeRegister(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref, uint16_t cfgBits){
                setValue(ref, cfgBits);
                writeRegister(ref);
            }

        template<typename T, unsigned mOffset, unsigned bOffset, unsigned mask, bool msbRight>
            void writeRegister(Field<T, mOffset, bOffset, mask, msbRight> Fei4GlobalCfg::*ref){
                wrRegister(chipId, getAddr(ref), cfg[getAddr(ref)]);
            }
    private:
        unsigned chipId;
};

#endif
