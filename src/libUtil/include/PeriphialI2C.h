#ifndef PERIPHIALI2C_H
#define PERIPHIALI2C_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: I2C master interface
// # Comment: Base class for I2C devices
// ################################

#include <iostream>

#include "SpecCom.h"

class PeriphialI2C {
    public:
    protected:
        PeriphialI2C(SpecCom *arg_spec);

        void init();
        int writeReg(uint32_t dev_addr, uint32_t reg_addr, uint32_t value);
        int readReg(uint32_t dev_addr, uint32_t reg_addr, uint32_t *value);

        int setAddr(uint32_t dev_addr, uint32_t reg_addr);
        int writeData(uint32_t value);
        int setupWrite(uint32_t dev_addr);
        int setupRead(uint32_t dev_addr);
        int readData(uint32_t *value);
        int sendNack();
    private:
        SpecCom *spec;
        uint32_t last_dev_addr;
        uint32_t last_reg_addr;
        //uint32_t last_rw;
        int checkTip();
};

#endif
