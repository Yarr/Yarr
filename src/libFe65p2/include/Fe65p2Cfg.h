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
#include "json.hpp"

#include "Constants.h"
#include "Units.h"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Fe65p2Cfg : public FrontEndCfg, public Fe65p2GlobalCfg, public Fe65p2PixelCfg{
    public:
        Fe65p2Cfg() {
            cap = 1.18;
            vcal_slope = 0.564;
            vcal_offset = 0.011;
        }
        
        ~Fe65p2Cfg() {

        }
        
        double toCharge(double vcal) {
            // Q = C*V
            return (cap * Unit::Femto)*(((Unit::Milli*vcal_slope)*vcal)+(vcal_offset*Unit::Milli))/Physics::ElectronCharge;
        }

        // Only one cap
        double toCharge(double vcal, bool scap, bool lcap) {
            return this->toCharge(vcal);
        }

        unsigned toVcal(double charge) {
            // V = Q/C
            return floor((((charge*Physics::ElectronCharge)/(cap * Unit::Femto))-(vcal_offset*Unit::Milli))/(vcal_slope*Unit::Milli));
        }
        
        void toFileJson(json &j);
        void fromFileJson(json &j);
        void toFileBinary(std::string) {};
        void fromFileBinary(std::string) {};
        void toFileBinary() {};
        void fromFileBinary() {};

    protected:
        std::string name;
    private:
        double cap; //fF
        double vcal_slope; //mV
        double vcal_offset; //mV

};

#endif
