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
#include "Constants.h"
#include "Units.h"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

class Fei4Cfg : public FrontEndCfg, public Fei4GlobalCfg, public Fei4PixelCfg {
    public:
        Fei4Cfg() {
            chipId = 0;
            sCap = 1.9;
            lCap = 3.8;
            vcalOffset = 0;
            vcalSlope = 1.5;
        }

        double toCharge(double vcal) override {return this->toCharge(vcal, true, true);}
        double toCharge(double vcal, bool sCapOn=true, bool lCapOn=true) override {
            // Q = C*V
            double C = 0;
            if (sCapOn) C += sCap*Unit::Femto;
            if (lCapOn) C += lCap*Unit::Femto;
            double V = ((vcalOffset*Unit::Milli) + ((vcalSlope*Unit::Milli)*vcal))/Physics::ElectronCharge;
            return C*V;
        }

        unsigned toVcal(double charge, bool sCapOn=true, bool lCapOn=true) {
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

        unsigned getChipId();
        void setChipId(unsigned chipId);

        void toFileBinary(std::string filename) override;
        void toFileBinary() override;
        void fromFileBinary(std::string filename) override;
        void fromFileBinary() override;
        void toFileXml(tinyxml2::XMLDocument *doc);
        void toFileJson(json &j) override;
        void fromFileJson(json &j) override;

    protected:
        unsigned chipId;
        std::string cfgName;

    private:
        double sCap; // fF
        double lCap; // fF
        double vcalOffset; // mV
        double vcalSlope; // mV
};

#endif
