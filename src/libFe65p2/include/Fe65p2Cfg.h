#ifndef FE65P2CFG_H
#define FE65P2CFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-65-P2 Library
// # Comment: FE65P2 config Base class
// ################################

#include <string>
#include <cmath>

#include "FrontEnd.h"
#include "Fe65p2GlobalCfg.h"
#include "Fe65p2PixelCfg.h"


#include "Constants.h"
#include "Units.h"

#include "storage.hpp"

class Fe65p2Cfg : public FrontEndCfg, public Fe65p2GlobalCfg, public Fe65p2PixelCfg{
    public:
        Fe65p2Cfg() {
            cap = 1.18;
            vcal_slope = 0.564;
            vcal_offset = 0.011;
        }
        
        ~Fe65p2Cfg() override = default;
        
        double toCharge(double vcal) override {
            // Q = C*V
            return (cap * Unit::Femto)*(((Unit::Milli*vcal_slope)*vcal)+(vcal_offset*Unit::Milli))/Physics::ElectronCharge;
        }

        // Only one cap
        double toCharge(double vcal, bool scap, bool lcap) override {
            return this->toCharge(vcal);
        }

        unsigned toVcal(double charge) const {
            // V = Q/C
            return floor((((charge*Physics::ElectronCharge)/(cap * Unit::Femto))-(vcal_offset*Unit::Milli))/(vcal_slope*Unit::Milli));
        }
        
        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    protected:
        std::string name;
    private:
        float cap; //fF
        float vcal_slope; //mV
        float vcal_offset; //mV

};

#endif
