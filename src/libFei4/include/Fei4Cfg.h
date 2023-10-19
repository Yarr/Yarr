#ifndef FEI4CFG
#define FEI4CFG

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Base class
// ################################

#include <iostream>
#include <fstream>
#include <cmath>

#include "FrontEnd.h"
#include "Fei4GlobalCfg.h"
#include "Fei4PixelCfg.h"

#include "Constants.h"
#include "Units.h"

#include "storage.hpp"

class Fei4Cfg : public FrontEndCfg, public Fei4GlobalCfg, public Fei4PixelCfg {
    public:
        Fei4Cfg() {
            chipId = 0;
            sCap = 1.9;
            lCap = 3.8;
            vcalOffset = 0;
            vcalSlope = 1.5;
        }

        void maskPixel(unsigned col, unsigned row) override {
            this->setEn(col+1, row+1, 0);
            this->setHitbus(col+1, row+1, 1);
        }

        unsigned getPixelEn(unsigned col, unsigned row) override {
	    return this->getEn(col, row);
        }

        void enableAll() override;

        double toCharge(double vcal) override {return this->toCharge(vcal, true, true);}
        double toCharge(double vcal, bool sCapOn=true, bool lCapOn=true) override {
            // Q = C*V
            double C = 0;
            if (sCapOn) C += sCap*Unit::Femto;
            if (lCapOn) C += lCap*Unit::Femto;
            double V = ((vcalOffset*Unit::Milli) + ((vcalSlope*Unit::Milli)*vcal))/Physics::ElectronCharge;
            return C*V;
        }

        unsigned toVcal(double charge, bool sCapOn=true, bool lCapOn=true) const {
            // V = Q/C
            double C = 0;
            if (sCapOn) C += sCap*Unit::Femto;
            if (lCapOn) C += lCap*Unit::Femto;
            if (C==0) return 0;
            double V = (charge*Physics::ElectronCharge)/C;
            return (unsigned) round((V - vcalOffset*Unit::Milli)/(vcalSlope*Unit::Milli));
        }

        void setScap(double c) {sCap = c;}
        void setLcap(double c) {lCap = c;}
        void setVcalSlope(double s) {vcalSlope = s;}
        void setVcalOffset(double o) {vcalOffset = o;}

        unsigned getChipId() const;
        void setChipId(unsigned chipId);

        void writeConfig(json &j) override;
        void loadConfig(const json &j) override;

    protected:
        unsigned chipId;
        std::string cfgName;

    private:
        float sCap; // fF
        float lCap; // fF
        float vcalOffset; // mV
        float vcalSlope; // mV
};

#endif
