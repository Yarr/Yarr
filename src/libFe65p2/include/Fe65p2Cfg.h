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

#include "Fe65p2GlobalCfg.h"
#include "Fe65p2PixelCfg.h"

#define ELECTRON_CHARGE 1.602e-19

class Fe65p2Cfg : public Fe65p2GlobalCfg, public Fe65p2PixelCfg{
    public:
        Fe65p2Cfg() {
            cap = 1.18;
        }
        
        ~Fe65p2Cfg() {

        }
        
        double toCharge(double v) {
            // Q = C*V
            return (cap * 1.0e-15)*(v)/ELECTRON_CHARGE;
        }

        double toV(double charge) {
            // V = Q/C
            return (charge*ELECTRON_CHARGE)/cap;
        }

    protected:
    private:
        double cap; //fF

};

#endif
