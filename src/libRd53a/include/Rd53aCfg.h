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



class Rd53aCfg : public FrontEndCfg, public Rd53aGlobalCfg, public Rd53aPixelCfg {
    public:
        Rd53aCfg();

        void maskPixel(unsigned col, unsigned row) override {
            this->setEn(col, row, 0);
            this->setHitbus(col, row, 0);
        }

        unsigned getPixelEn(unsigned col, unsigned row) override {
            return this->getEn(col, row);
        }

        void enableAll() override;

        /**
         * Obtain the corresponding charge [e] from the input VCal
         */
        double toCharge(double vcal) override;
	
        /**
         * Obtain the corresponding charge [e] from the input VCal, small&large capacitances(?)
         * Not fully implmented yet.
         */
        double toCharge(double vcal, bool sCap, bool lCap) override;

        /**
         * Obtain the corresponding VCal from the input charge [e]
         */
        unsigned toVcal(double charge);
        
        /**
         * Format converters
         * These can be possibly templated:
         * template<YARR::IOFormat IN, YARR::IOFormat OUT> void convert( IN&); ?
         */
        void writeConfig(json &j) override;
        void loadConfig(const json &) override;
        
        float ADCtoV (uint16_t ADC);
        float VtoTemp (float V, uint16_t Sensor, bool isRadSensor);
        
        /**
         * set the chip ID
         */
        void setChipId(unsigned id);

        /**
         * get the chip ID
         */
        unsigned getChipId() const;

    protected:
        unsigned m_chipId;

    private:
        float m_injCap; //fF
        std::array<float, 4> m_vcalPar; //mV, [0] + [1]*x + [2]*x^2 + [3]*x^3
        std::array<float, 2> m_ADCcalPar; //mV, [0] + [1]*x
        std::array< std::array<float, 2>, 4 > m_TempSenPar;
        std::array< std::array<float, 2>, 4 > m_RadSenPar;
};

#endif
