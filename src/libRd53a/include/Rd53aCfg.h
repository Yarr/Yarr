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
#include "Rd53aPixelCfg.h"

#include "json.hpp"

class Rd53aCfg : public FrontEndCfg, public Rd53aGlobalCfg, public Rd53aPixelCfg {
    public:
        Rd53aCfg() {
            m_chipId = 0;
            m_vcalPar[0] = 0;
            m_vcalPar[1] = 1.17;
            m_vcalPar[0] = 0;
            m_vcalPar[0] = 0;
            m_injCap = 8.5;
        }

        double toCharge(double) {return 0;}
        double toCharge(double, bool, bool) {return 0;}

        void toFileJson(json&);
        void fromFileJson(json&);
        void toFileBinary(std::string) {};
        void fromFileBinary(std::string) {};
        void toFileBinary() {};
        void fromFileBinary() {};

        void setChipId(unsigned id) {
            m_chipId = id;
        }

    protected:
        unsigned m_chipId;

    private:
        double m_injCap; //fF
        std::array<double, 4> m_vcalPar; //mV, [0] + [1]*x + [2]*x^2 + [3]*x^3
};

#endif
