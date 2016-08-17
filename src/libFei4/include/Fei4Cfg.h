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
#include "tinyxml2.h"
#include "json.hpp"

using json = nlohmann::json;

#define ELECTRON_CHARGE 1.602e-19

class Fei4Cfg : public FrontEndCfg, public Fei4GlobalCfg, public Fei4PixelCfg {
    public:
        Fei4Cfg() {
            chipId = 0;
            sCap = 1.9;
            lCap = 3.8;
            vcalOffset = 0;
            vcalSlope = 1.5;
        }

        double toCharge(double vcal) {return this->toCharge(vcal, true, true);}
        double toCharge(double vcal, bool sCapOn=true, bool lCapOn=true) {
            // Q = C*V
            double C = 0;
            if (sCapOn) C += sCap*1e-15;
            if (lCapOn) C += lCap*1e-15;
            double V = ((vcalOffset*1e-3) + ((vcalSlope*1e-3)*vcal))/ELECTRON_CHARGE;
            return C*V;
        }

        unsigned toVcal(double charge, bool sCapOn=true, bool lCapOn=true) {
            // V = Q/C
            double C = 0;
            if (sCapOn) C += sCap*1e-15;
            if (lCapOn) C += lCap*1e-15;
            if (C==0) return 0;
            double V = (charge*ELECTRON_CHARGE)/C;
            return (unsigned) round((V - vcalOffset*1e-3)/(vcalSlope*1e-3));
        }

        void setScap(double c) {sCap = c;}
        void setLcap(double c) {lCap = c;}
        void setVcalSlope(double s) {vcalSlope = s;}
        void setVcalOffset(double o) {vcalOffset = o;}

		unsigned getChipId();
		void setChipId(unsigned chipId);
        
        void toFileBinary(std::string filename);
        void toFileBinary();
        void fromFileBinary(std::string filename);
        void fromFileBinary();
        void toFileXml(tinyxml2::XMLDocument *doc);
        void toFileJson(json &j);
        void fromFileJson(json &j);

    protected:
        std::string name;
        unsigned chipId;
        std::string cfgName;

    private:
        double sCap; // fF
        double lCap; // fF
        double vcalOffset; // mV
        double vcalSlope; // mV
};

#endif
