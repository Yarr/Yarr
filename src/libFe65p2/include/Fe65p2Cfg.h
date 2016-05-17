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

#include "Fe65p2GlobalCfg.h"
#include "Fe65p2PixelCfg.h"

#define ELECTRON_CHARGE 1.602e-19

class Fe65p2Cfg : public Fe65p2GlobalCfg, public Fe65p2PixelCfg{
    public:
        Fe65p2Cfg() {
            cap = 1.18;
            vcal_slope = 0.564e-3;
            vcal_offset = 0.011e-3;
        }
        
        ~Fe65p2Cfg() {

        }
        
        double toCharge(unsigned vcal) {
            // Q = C*V
            return (cap * 1.0e-15)*((vcal_slope*vcal)+vcal_offset)/ELECTRON_CHARGE;
        }

        unsigned toVcal(double charge) {
            // V = Q/C
            return floor((((charge*ELECTRON_CHARGE)/cap)-vcal_offset)/vcal_slope);
        }

    protected:
    private:
        double cap; //fF
        double vcal_slope; //V
        double vcal_offset; //V

};

#endif
