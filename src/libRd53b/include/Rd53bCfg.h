#ifndef RD53BCFG_H
#define RD53BCFG_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B config base class
// # Date: May 2020
// ################################

#include "FrontEnd.h"
#include "Constants.h"
#include "Units.h"

#include "Rd53bGlobalCfg.h"
#include "Rd53bPixelCfg.h"

class Rd53bCfg : public FrontEndCfg, public Rd53bGlobalCfg, public Rd53bPixelCfg {
    public:
        Rd53bCfg();

        void maskPixel(unsigned col, unsigned row) override {
            this->setEn(col, row, 0);
            this->setHitbus(col, row, 0);
        }
        
        unsigned getPixelEn(unsigned col, unsigned row) override {
            return this->getEn(col, row);
        }

        void enableAll() override;

        double toCharge(double vcal) override;
        double toCharge(double vcal, bool sCap, bool lCap) override;
        unsigned toVcal(double charge);

        void writeConfig(json &cfg) override;
        void loadConfig(const json &cfg) override;

        void setChipId(unsigned id);
        unsigned getChipId() const;

        std::pair<float, std::string> convertAdc(uint16_t ADC, bool meas_curr) override;
        float adcToV(uint16_t ADC);
        float adcToI(uint16_t ADC);
        float vToTemp(float V, uint16_t Sensor = 1, bool isRadSensor = false); // Read temperature from resistance sensor (not ready for ITkPix-V1)
        float readNtcTemp(float R, bool in_kelvin = false);                    // Read temperature from NTC

        // 0: digital SLDO, 1: analog SLDO, 2: center, 3: other
        enum TransSensor
        {
            DSLDO = 0,
            ASLDO = 1,
            ACB = 2,
            Other = 3
        };
        float readMosTemp(float deltaV, TransSensor sensor, bool in_kelvin = false) const;               // Read temperature from MOS

    protected:
        unsigned m_chipId;

    private:
        float m_injCap;                   //fF
        std::array<float, 2> m_vcalPar;   //mV, [0] + [1]*x
        std::array<float, 3> m_adcCalPar; //mV, [0] + [1]*x, R_IMUX = [2]
        std::array<std::array<float, 2>, 3> m_radSenPar;  // Not used yet
        std::array<std::array<float, 2>, 3> m_tempSenPar;  // Not used yet
        std::array<float, 3> m_nf;               // MOS temperature sensor <digital SLDO, analog SLDO, center>
        int m_irefTrim;                                 // Iref trim bit
        float m_kSenseInA;                                 // Multiplicator of IinA
        float m_kSenseInD;                                 // Multiplicator of IinD
        float m_kSenseShuntA;                              // Multiplicator of IshuntA
        float m_kSenseShuntD;                              // Multiplicator of IshuntD
        float m_kShuntA;                              // kFactor of the analog shunt circuit
        float m_kShuntD;                              // kFactor of the digital shunt circuit
        std::array<float, 3> m_ntcCalPar;                 // Steinhart coefficients
};

#endif
