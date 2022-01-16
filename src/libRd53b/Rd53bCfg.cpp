// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B config base class
// # Date: May 2020
// ################################

#include "Rd53bCfg.h"

#include <cmath>

Rd53bCfg::Rd53bCfg() :
    m_chipId(15),
    m_vcalPar({{0.46, 0.2007}}),
    m_ADCcalPar ( {{ 5.89435, 0.192043, 4.99e3 }} ),
    m_NTCcalPar({{7.489e-4, 2.769e-4, 7.0595e-8}}),
    m_MOScalPar(1.264),
    m_injCap(7.5)
{}

double Rd53bCfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
    //double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
    double V = (m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge; // Note no offset applied
    return V*m_injCap*Unit::Femto;
}

double Rd53bCfg::toCharge(double vcal, bool sCap, bool lCap) {
    return toCharge(vcal);
}

unsigned Rd53bCfg::toVcal(double charge) {
    double V= (charge*Physics::ElectronCharge)/(m_injCap*Unit::Femto);
    unsigned vcal = (unsigned) round((V)/(m_vcalPar[1]*Unit::Milli)); // Note: no offset applied
    return vcal;
}

void Rd53bCfg::writeConfig(json &j) {
    // General Paramters
    j["RD53B"]["Parameter"]["Name"] = name;
    j["RD53B"]["Parameter"]["ChipId"] = m_chipId;
    j["RD53B"]["Parameter"]["InjCap"] = m_injCap;
    j["RD53B"]["Parameter"]["MOScalPar"] = m_MOScalPar;
    for(unsigned  i=0;i<m_vcalPar.size();i++)  
        j["RD53B"]["Parameter"]["VcalPar"][i]= m_vcalPar[i];
    for(unsigned  i=0;i<m_ADCcalPar.size();i++)  
        j["RD53B"]["Parameter"]["ADCcalPar"][i]= m_ADCcalPar[i];
    for (unsigned i = 0; i < m_NTCcalPar.size(); i++)
        j["RD53B"]["Parameter"]["SteinhartCoeffs"][i] = m_NTCcalPar[i];
    Rd53bGlobalCfg::writeConfig(j);
    Rd53bPixelCfg::writeConfig(j);
}

void Rd53bCfg::loadConfig(const json &j) {
    if (j.contains({"RD53B","Parameter","Name"}))
        name = j["RD53B"]["Parameter"]["Name"];
    if (j.contains({"RD53B","Parameter","ChipId"}))
        m_chipId = j["RD53B"]["Parameter"]["ChipId"];
    if (j.contains({"RD53B","Parameter","InjCap"}))
        m_injCap = j["RD53B"]["Parameter"]["InjCap"];
    if (j.contains({"RD53B","Parameter","MOScalPar"}))
        m_MOScalPar = j["RD53B"]["Parameter"]["MOScalPar"];        
    if (j.contains({"RD53B","Parameter","VcalPar"}))
        for(unsigned  i=0;i<m_vcalPar.size();i++)
            m_vcalPar[i] = j["RD53B"]["Parameter"]["VcalPar"][i];
    if (j.contains({"RD53B","Parameter","ADCcalPar"}))
        for(unsigned  i=0;i<m_ADCcalPar.size();i++)
            m_ADCcalPar[i] = j["RD53B"]["Parameter"]["ADCcalPar"][i];
    if (j.contains({"RD53B","Parameter","SteinhartCoeffs"}))
        for (unsigned i = 0; i < m_NTCcalPar.size(); i++)
            m_NTCcalPar[i] = j["RD53B"]["Parameter"]["SteinhartCoeffs"][i];
    Rd53bGlobalCfg::loadConfig(j);
    Rd53bPixelCfg::loadConfig(j);
}

void Rd53bCfg::setChipId(unsigned id) {
    m_chipId = id;
}

unsigned Rd53bCfg::getChipId() const {
    return m_chipId;
}

float Rd53bCfg::ADCtoV(uint16_t ADC) {
    return (float(ADC)*m_ADCcalPar[1]+m_ADCcalPar[0])*Unit::Milli;
}

float Rd53bCfg::ADCtoI(uint16_t ADC) {
    return ADCtoV(ADC)/m_ADCcalPar[2];
}

float Rd53bCfg::VtoTemp(float V, uint16_t Sensor, bool isRadSensor) {
    float p0 = 0;
    float p1 = 0;

    if (isRadSensor){
        p0 = m_RadSenPar[Sensor][0];
        p1 = m_RadSenPar[Sensor][1];
    }
    else {
        p0 = m_TempSenPar[Sensor][0];
        p1 = m_TempSenPar[Sensor][1];
    }

    return (V*p1+p0);
}

float Rd53bCfg::readNTCTemp(float R, bool in_kelvin)
{
    float a = m_NTCcalPar[0];
    float b = m_NTCcalPar[1];
    float c = m_NTCcalPar[2];
    float logres = log(R);
    float tK = 1.0 / (a + b * logres + c * pow(logres, 3));
    if (in_kelvin) return tK;
    return tK - 273.15;
}

float Rd53bCfg::readMOSTemp(float deltaV, bool in_kelvin) const
{
    // Convert voltage difference into temperature from https://indico.cern.ch/event/1011941/contributions/4278988/attachments/2210633/3741190/RD53B_calibatrion_sensor_temperature.pdf
    float Nf = m_MOScalPar;
    const float kB = 1.38064852e-23, e = 1.602e-19; // Boltzmann constant and electron charge
    float tK = deltaV * e / (Nf * kB * log(15.));
    if (in_kelvin)
        return tK;
    return tK - 273.15;
}
