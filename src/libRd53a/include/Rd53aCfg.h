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
#include "Constants.h"
#include "Units.h"

#include "json.hpp"

class Rd53aCfg : public FrontEndCfg, public Rd53aGlobalCfg, public Rd53aPixelCfg {
    public:
        Rd53aCfg();

        /**
         * Obtain the corresponding charge [e] from the input VCal
         */
        double toCharge(double vcal);
    
        /**
         * Obtain the corresponding charge [e] from the input VCal, small&large capacitances(?)
         * Not fully implmented yet.
         */
        double toCharge(double vcal, bool sCap, bool lCap);

        /**
         * Obtain the corresponding VCal from the input charge [e]
         */
        unsigned toVcal(double charge);
        /**
         * Format converters
         * These can be possibly templated:
         * template<YARR::IOFormat IN, YARR::IOFormat OUT> void convert( IN&); ?
         */
        void toFileJson(json&);
        void fromFileJson(json&);
        void toFileBinary(std::string) {};
        void fromFileBinary(std::string) {};
        void toFileBinary() {};
        void fromFileBinary() {};

        /**
         * set the chip ID
         */
        void setChipId(unsigned id);

    protected:
        unsigned m_chipId;

    private:
        double m_injCap; //fF
        std::array<double, 4> m_vcalPar; //mV, [0] + [1]*x + [2]*x^2 + [3]*x^3
};

#endif
