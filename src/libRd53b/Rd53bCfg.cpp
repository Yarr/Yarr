// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53B config base class
// # Date: May 2020
// ################################

#include "Rd53bCfg.h"
#include "logging.h"

#include <cmath>

// Create logger
namespace {
  auto logger = logging::make_log("Rd53bCfg");
}

Rd53bCfg::Rd53bCfg() :
    m_chipId(15),
    m_vcalPar({{0.46, 0.2007}}),
    m_adcCalPar ( {{ 5.89435, 0.192043, 4.99e3 }} ),
    m_ntcCalPar({{7.489e-4, 2.769e-4, 7.0595e-8}}),
    m_nf({{1.264, 1.264, 1.264}}),
    m_injCap(7.5),
    m_irefTrim(15),
    m_kSenseInA(21000),
    m_kSenseInD(21000),
    m_kSenseShuntA(26000),
    m_kSenseShuntD(26000),
    m_kShuntA(1040),
    m_kShuntD(1040)
{}

void Rd53bCfg::enableAll() {
    logger->info("Resetting enable/hitbus pixel mask to all enabled!");
    for (unsigned int col = 0; col < n_Col; col++) {
        for (unsigned row = 0; row < n_Row; row ++) {
            setEn(col, row, 1);
            setHitbus(col, row, 1);
        }
    }
}

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
    j["RD53B"]["Parameter"]["NfDSLDO"] = m_nf[0];
    j["RD53B"]["Parameter"]["NfASLDO"] = m_nf[1];
    j["RD53B"]["Parameter"]["NfACB"] = m_nf[2];
    j["RD53B"]["Parameter"]["IrefTrim"] = m_irefTrim;
    j["RD53B"]["Parameter"]["KSenseInA"] = m_kSenseInA;
    j["RD53B"]["Parameter"]["KSenseInD"] = m_kSenseInD;
    j["RD53B"]["Parameter"]["KSenseShuntA"] = m_kSenseShuntA;
    j["RD53B"]["Parameter"]["KSenseShuntD"] = m_kSenseShuntD;
    j["RD53B"]["Parameter"]["KShuntA"] = m_kShuntA;
    j["RD53B"]["Parameter"]["KShuntD"] = m_kShuntD;
    for(unsigned  i=0;i<m_vcalPar.size();i++)  
        j["RD53B"]["Parameter"]["VcalPar"][i]= m_vcalPar[i];
    j["RD53B"]["Parameter"]["EnforceNameIdCheck"] = enforceChipIdInName;
    for(unsigned  i=0;i<m_adcCalPar.size();i++)  
        j["RD53B"]["Parameter"]["ADCcalPar"][i]= m_adcCalPar[i];
    for (unsigned i = 0; i < m_ntcCalPar.size(); i++)
        j["RD53B"]["Parameter"]["NtcCalPar"][i] = m_ntcCalPar[i];
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
   
    if (j.contains({"RD53B", "Parameter","EnforceNameIdCheck"}))
        enforceChipIdInName = j["RD53B"]["Parameter"]["EnforceNameIdCheck"];
    
    if (j.contains({"RD53B","Parameter","NfDSLDO"}))
        m_nf[0] = j["RD53B"]["Parameter"]["NfDSLDO"];

    if (j.contains({"RD53B","Parameter","NfASLDO"}))
        m_nf[1] = j["RD53B"]["Parameter"]["NfASLDO"];

    if (j.contains({"RD53B","Parameter","NfACB"}))
        m_nf[2] = j["RD53B"]["Parameter"]["NfACB"];

    if (j.contains({"RD53B","Parameter","IrefTrim"}))
        m_irefTrim = j["RD53B"]["Parameter"]["IrefTrim"];

    if (j.contains({"RD53B","Parameter","KSenseInA"}))
        m_kSenseInA = j["RD53B"]["Parameter"]["KSenseInA"];

    if (j.contains({"RD53B","Parameter","KSenseInD"}))
        m_kSenseInD = j["RD53B"]["Parameter"]["KSenseInD"];

    if (j.contains({"RD53B","Parameter","KSenseShuntA"}))
        m_kSenseShuntA = j["RD53B"]["Parameter"]["KSenseShuntA"];

    if (j.contains({"RD53B","Parameter","KSenseShuntD"}))
        m_kSenseShuntD = j["RD53B"]["Parameter"]["KSenseShuntD"];

    if (j.contains({"RD53B","Parameter","KShuntA"}))
        m_kShuntA = j["RD53B"]["Parameter"]["KShuntA"];

    if (j.contains({"RD53B","Parameter","KShuntD"}))
        m_kShuntD = j["RD53B"]["Parameter"]["KShuntD"];
    
    if (j.contains({"RD53B","Parameter","VcalPar"}))
        if (j["RD53B"]["Parameter"]["VcalPar"].size() == m_vcalPar.size()) {
            for(unsigned  i=0;i<m_vcalPar.size();i++)
                m_vcalPar[i] = j["RD53B"]["Parameter"]["VcalPar"][i];
        }

    if (j.contains({"RD53B","Parameter","ADCcalPar"}))
        if (j["RD53B"]["Parameter"]["ADCcalPar"].size() == m_adcCalPar.size()) {
            for(unsigned  i=0;i<m_adcCalPar.size();i++)
                m_adcCalPar[i] = j["RD53B"]["Parameter"]["ADCcalPar"][i];
        }

    if (j.contains({"RD53B","Parameter","NtcCalPar"}))
        if (j["RD53B"]["Parameter"]["NtcCalPar"].size() == m_ntcCalPar.size()) {
            for (unsigned i = 0; i < m_ntcCalPar.size(); i++)
                m_ntcCalPar[i] = j["RD53B"]["Parameter"]["NtcCalPar"][i];
        }

    Rd53bGlobalCfg::loadConfig(j);
    Rd53bPixelCfg::loadConfig(j);
}

void Rd53bCfg::setChipId(unsigned id) {
    m_chipId = id;
}

unsigned Rd53bCfg::getChipId() const {
    return m_chipId;
}

float Rd53bCfg::adcToV(uint16_t ADC) {
    return (float(ADC)*m_adcCalPar[1]+m_adcCalPar[0])*Unit::Milli;
}

float Rd53bCfg::adcToI(uint16_t ADC) {
    return adcToV(ADC)/m_adcCalPar[2];
}

std::pair<float, std::string> Rd53bCfg::convertAdc(uint16_t ADC, bool meas_curr) {
    if (meas_curr) return std::make_pair(adcToI(ADC)/1e-6, "uA");
    else return std::make_pair(adcToV(ADC), "V");
}

float Rd53bCfg::vToTemp(float V, uint16_t Sensor, bool isRadSensor) {
    float p0 = 0;
    float p1 = 0;

    if (isRadSensor){
        p0 = m_radSenPar[Sensor][0];
        p1 = m_radSenPar[Sensor][1];
    }
    else {
        p0 = m_tempSenPar[Sensor][0];
        p1 = m_tempSenPar[Sensor][1];
    }

    return (V*p1+p0);
}

float Rd53bCfg::readNtcTemp(float R, bool in_kelvin)
{
    float a = m_ntcCalPar[0];
    float b = m_ntcCalPar[1];
    float c = m_ntcCalPar[2];
    float logres = log(R);
    float tK = 1.0 / (a + b * logres + c * pow(logres, 3));
    if (in_kelvin) return tK;
    return tK - 273.15;
}

float Rd53bCfg::readMosTemp(float deltaV, TransSensor sensor, bool in_kelvin) const
{
    // Convert voltage difference into temperature from https://indico.cern.ch/event/1011941/contributions/4278988/attachments/2210633/3741190/RD53B_calibatrion_sensor_temperature.pdf
    if (sensor == Other) {
        logger->error("Not a transistor sensor");
    }
    float Nf = 0;
    Nf = m_nf[static_cast<int>(sensor)];
    const float kB = 1.38064852e-23, e = 1.602e-19; // Boltzmann constant and electron charge
    float tK = deltaV * e / (Nf * kB * log(15.));
    if (in_kelvin)
        return tK;
    return tK - 273.15;
}
