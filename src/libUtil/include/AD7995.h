#ifndef AD7995_H
#define AD7995_H
// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: AD7995 ADC
// # Comment: I2C interface to ADCs
// ################################

#include <iostream>

#include "SpecController.h"
#include "PeriphialI2C.h"

class AD7995 : public PeriphialI2C {
    public:
        AD7995(SpecController *arg_spec);
        ~AD7995();
        void setActiveChannels(bool ch1, bool ch2, bool ch3, bool ch4);
        double read();

    private:
        uint32_t dev_addr;
        unsigned ch_cnt;
};

#endif
