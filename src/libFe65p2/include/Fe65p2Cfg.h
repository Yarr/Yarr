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

using json = nlohmann::json;

#define ELECTRON_CHARGE 1.602e-19

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
            return (cap * 1.0e-15)*(((1.0e-3*vcal_slope)*vcal)+(vcal_offset*1.0e-3))/ELECTRON_CHARGE;
        }

        unsigned toVcal(double charge) {
            // V = Q/C
            return floor((((charge*ELECTRON_CHARGE)/(cap * 1.0e-15))-(vcal_offset*1.0e-3))/(vcal_slope*1.0e-3));
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
