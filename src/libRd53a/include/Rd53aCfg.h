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

#include "Constants.h"
#include "Units.h"

class Rd53aCfg : public FrontEndCfg, public Rd53aGlobalCfg, public Rd53aPixelCfg {
    public:
        Rd53aCfg() {
            m_chipId = 0;
            m_vcalPar[0] = -1.0;
            m_vcalPar[1] = 0.2;
            m_vcalPar[2] = 0;
            m_vcalPar[3] = 0;
            m_injCap = 8.2;
        }

        double toCharge(double vcal) {
            // Q = C*V
            // Linear is good enough
            double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
            return V*m_injCap*Unit::Femto;
        }
        double toCharge(double vcal, bool sCap, bool lCap) {return this->toCharge(vcal);}

        unsigned toVcal(double charge) {
            double V= (charge*Physics::ElectronCharge)/(m_injCap*Unit::Femto);
            unsigned vcal = (unsigned) round((V-(m_vcalPar[0]*Unit::Milli))/(m_vcalPar[1]*Unit::Milli));
            return vcal;
        }

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
