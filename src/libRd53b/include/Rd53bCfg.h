#ifndef RD53BCFG_H
#define RD53BCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B config base class
// # Date: May 2020
// ################################

#include "FrontEnd.h"
#include "Constants.h"
#include "Units.h"

#include "Rd53bGlobalCfg.h"
#include "Rd53bPixelCfg.h"

class Rd53bCfg : public FrontEndCfg, public Rd53bGlobalCfg, public Rd53bPixelCfg {
    public:
        Rd53bCfg();

        double toCharge(double vcal);
        double toCharge(double vcal, bool sCap, bool lCap);
        unsigned toVcal(double charge);

        void toFileJson(json &cfg);
        void fromFileJson(json &cfg);

        void setChipId(unsigned id);
        unsigned getChipId();

    protected:
        unsigned m_chipId;
    
    private:
        float m_injCap; //fF
        std::array<float, 2> m_vcalPar; //mV, [0] + [1]*x
};

#endif
