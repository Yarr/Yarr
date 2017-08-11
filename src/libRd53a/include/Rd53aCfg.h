#ifndef RD53ACFG_H
#define RD53ACFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Comment: RD53A base class
// # Date: Jun 2017
// #################################

#include <iostream>

#include "FrontEnd.h"
#include "Rd53aGlobalCfg.h"
#include "json.hpp"

class Rd53aCfg : public FrontEndCfg, public Rd53aGlobalCfg {
    public:
        Rd53aCfg() {}

        double toCharge(double) {return 0;}

        void toFileJson(json&) {};
        void fromFileJson(json&) {};
        void toFileBinary(std::string) {};
        void fromFileBinary(std::string) {};
        void toFileBinary() {};
        void fromFileBinary() {};

    protected:
    private:
};

#endif
