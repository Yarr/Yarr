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
    MASK_ALL     = 0xFFFFFFFF,
    MASK_HALF    = 0x55555555,
    MASK_QUARTER = 0x11111111,
    MASK_EIGHTH  = 0x01010101
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
