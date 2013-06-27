#ifndef SPECCONTROLLER_H
#define SPECCONTROLLER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: SPEC cpp library
// # Comment: Original driver taken from Marcus Guillermo
// #          Modified for SPEC card
// ################################

#include <SpecDevice.h>

class SpecController {
    public:
        SpecController();
        ~SpecController();
        
        void init();

    private:
        SpecDevice *spec;
        void *bar0, *bar4;

};

#endif
