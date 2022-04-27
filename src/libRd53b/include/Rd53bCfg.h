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

        double toCharge(double vcal) override;
        double toCharge(double vcal, bool sCap, bool lCap) override;
        unsigned toVcal(double charge);

        void writeConfig(json &cfg) override;
        void loadConfig(const json &cfg) override;

        void setChipId(unsigned id);
        unsigned getChipId() const;

        float adcToV(uint16_t ADC);
        float adcToI(uint16_t ADC);
        float vToTemp(float V, uint16_t Sensor = 1, bool isRadSensor = false); // Read temperature from resistance sensor (not ready for ITkPix-V1)
        float readNtcTemp(float R, bool in_kelvin = false);                    // Read temperature from NTC
        float readMosTemp(float deltaV, bool in_kelvin = false) const;               // Read temperature from MOS

    protected:
        unsigned m_chipId;

    private:
        float m_injCap;                   //fF
        std::array<float, 2> m_vcalPar;   //mV, [0] + [1]*x
        std::array<float, 3> m_adcCalPar; //mV, [0] + [1]*x, R_IMUX = [2]
        std::array<std::array<float, 2>, 3> m_radSenPar;  // Not used yet
        std::array<std::array<float, 2>, 3> m_tempSenPar; // Not used yet
        float m_mosCalPar;                                // MOS temperature sensor. Only one parameter for ideality factor
        std::array<float, 3> m_ntcCalPar;                 // Steinhart coefficients
};

#endif
