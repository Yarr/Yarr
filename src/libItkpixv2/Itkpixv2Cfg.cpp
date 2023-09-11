// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: ITkPixV2 config base class
// # Date: Jul 2023
// ################################

#include "Itkpixv2Cfg.h"
#include "logging.h"

#include <cmath>

// Create logger
namespace {
  auto logger = logging::make_log("Itkpixv2Cfg");
}

Itkpixv2Cfg::Itkpixv2Cfg() :
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

double Itkpixv2Cfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
    //double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
    double V = (m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge; // Note no offset applied
    return V*m_injCap*Unit::Femto;
}

double Itkpixv2Cfg::toCharge(double vcal, bool sCap, bool lCap) {
    return toCharge(vcal);
}

unsigned Itkpixv2Cfg::toVcal(double charge) {
    double V= (charge*Physics::ElectronCharge)/(m_injCap*Unit::Femto);
    unsigned vcal = (unsigned) round((V)/(m_vcalPar[1]*Unit::Milli)); // Note: no offset applied
    return vcal;
}

void Itkpixv2Cfg::writeConfig(json &j) {
    // General Paramters
    j["ITKPIXV2"]["Parameter"]["Name"] = name;
    j["ITKPIXV2"]["Parameter"]["ChipId"] = m_chipId;
    j["ITKPIXV2"]["Parameter"]["InjCap"] = m_injCap;
    j["ITKPIXV2"]["Parameter"]["NfDSLDO"] = m_nf[0];
    j["ITKPIXV2"]["Parameter"]["NfASLDO"] = m_nf[1];
    j["ITKPIXV2"]["Parameter"]["NfACB"] = m_nf[2];
    j["ITKPIXV2"]["Parameter"]["IrefTrim"] = m_irefTrim;
    j["ITKPIXV2"]["Parameter"]["KSenseInA"] = m_kSenseInA;
    j["ITKPIXV2"]["Parameter"]["KSenseInD"] = m_kSenseInD;
    j["ITKPIXV2"]["Parameter"]["KSenseShuntA"] = m_kSenseShuntA;
    j["ITKPIXV2"]["Parameter"]["KSenseShuntD"] = m_kSenseShuntD;
    j["ITKPIXV2"]["Parameter"]["KShuntA"] = m_kShuntA;
    j["ITKPIXV2"]["Parameter"]["KShuntD"] = m_kShuntD;
    for(unsigned  i=0;i<m_vcalPar.size();i++)  
        j["ITKPIXV2"]["Parameter"]["VcalPar"][i]= m_vcalPar[i];
    j["ITKPIXV2"]["Parameter"]["EnforceNameIdCheck"] = enforceChipIdInName;
    for(unsigned  i=0;i<m_adcCalPar.size();i++)  
        j["ITKPIXV2"]["Parameter"]["ADCcalPar"][i]= m_adcCalPar[i];
    for (unsigned i = 0; i < m_ntcCalPar.size(); i++)
        j["ITKPIXV2"]["Parameter"]["NtcCalPar"][i] = m_ntcCalPar[i];
    Itkpixv2GlobalCfg::writeConfig(j);
    Itkpixv2PixelCfg::writeConfig(j);
}

void Itkpixv2Cfg::loadConfig(const json &j) {
    if (j.contains({"ITKPIXV2","Parameter","Name"}))
        name = j["ITKPIXV2"]["Parameter"]["Name"];
    
    if (j.contains({"ITKPIXV2","Parameter","ChipId"}))
        m_chipId = j["ITKPIXV2"]["Parameter"]["ChipId"];
    
    if (j.contains({"ITKPIXV2","Parameter","InjCap"}))
        m_injCap = j["ITKPIXV2"]["Parameter"]["InjCap"];
   
    if (j.contains({"ITKPIXV2", "Parameter","EnforceNameIdCheck"}))
        enforceChipIdInName = j["ITKPIXV2"]["Parameter"]["EnforceNameIdCheck"];
    
    if (j.contains({"ITKPIXV2","Parameter","NfDSLDO"}))
        m_nf[0] = j["ITKPIXV2"]["Parameter"]["NfDSLDO"];

    if (j.contains({"ITKPIXV2","Parameter","NfASLDO"}))
        m_nf[1] = j["ITKPIXV2"]["Parameter"]["NfASLDO"];

    if (j.contains({"ITKPIXV2","Parameter","NfACB"}))
        m_nf[2] = j["ITKPIXV2"]["Parameter"]["NfACB"];

    if (j.contains({"ITKPIXV2","Parameter","IrefTrim"}))
        m_irefTrim = j["ITKPIXV2"]["Parameter"]["IrefTrim"];

    if (j.contains({"ITKPIXV2","Parameter","KSenseInA"}))
        m_kSenseInA = j["ITKPIXV2"]["Parameter"]["KSenseInA"];

    if (j.contains({"ITKPIXV2","Parameter","KSenseInD"}))
        m_kSenseInD = j["ITKPIXV2"]["Parameter"]["KSenseInD"];

    if (j.contains({"ITKPIXV2","Parameter","KSenseShuntA"}))
        m_kSenseShuntA = j["ITKPIXV2"]["Parameter"]["KSenseShuntA"];

    if (j.contains({"ITKPIXV2","Parameter","KSenseShuntD"}))
        m_kSenseShuntD = j["ITKPIXV2"]["Parameter"]["KSenseShuntD"];

    if (j.contains({"ITKPIXV2","Parameter","KShuntA"}))
        m_kShuntA = j["ITKPIXV2"]["Parameter"]["KShuntA"];

    if (j.contains({"ITKPIXV2","Parameter","KShuntD"}))
        m_kShuntD = j["ITKPIXV2"]["Parameter"]["KShuntD"];
    
    if (j.contains({"ITKPIXV2","Parameter","VcalPar"}))
        if (j["ITKPIXV2"]["Parameter"]["VcalPar"].size() == m_vcalPar.size()) {
            for(unsigned  i=0;i<m_vcalPar.size();i++)
                m_vcalPar[i] = j["ITKPIXV2"]["Parameter"]["VcalPar"][i];
        }

    if (j.contains({"ITKPIXV2","Parameter","ADCcalPar"}))
        if (j["ITKPIXV2"]["Parameter"]["ADCcalPar"].size() == m_adcCalPar.size()) {
            for(unsigned  i=0;i<m_adcCalPar.size();i++)
                m_adcCalPar[i] = j["ITKPIXV2"]["Parameter"]["ADCcalPar"][i];
        }

    if (j.contains({"ITKPIXV2","Parameter","NtcCalPar"}))
        if (j["ITKPIXV2"]["Parameter"]["NtcCalPar"].size() == m_ntcCalPar.size()) {
            for (unsigned i = 0; i < m_ntcCalPar.size(); i++)
                m_ntcCalPar[i] = j["ITKPIXV2"]["Parameter"]["NtcCalPar"][i];
        }

    Itkpixv2GlobalCfg::loadConfig(j);
    Itkpixv2PixelCfg::loadConfig(j);
}

void Itkpixv2Cfg::setChipId(unsigned id) {
    m_chipId = id;
}

unsigned Itkpixv2Cfg::getChipId() const {
    return m_chipId;
}

float Itkpixv2Cfg::adcToV(uint16_t ADC) {
    return (float(ADC)*m_adcCalPar[1]+m_adcCalPar[0])*Unit::Milli;
}

float Itkpixv2Cfg::adcToI(uint16_t ADC) {
    return adcToV(ADC)/m_adcCalPar[2];
}

std::pair<float, std::string> Itkpixv2Cfg::convertAdc(uint16_t ADC, bool meas_curr) {
    if (meas_curr) return std::make_pair(adcToI(ADC)/1e-6, "uA");
    else return std::make_pair(adcToV(ADC), "V");
}

float Itkpixv2Cfg::vToTemp(float V, uint16_t Sensor, bool isRadSensor) {
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

float Itkpixv2Cfg::readNtcTemp(float R, bool in_kelvin)
{
    float a = m_ntcCalPar[0];
    float b = m_ntcCalPar[1];
    float c = m_ntcCalPar[2];
    float logres = log(R);
    float tK = 1.0 / (a + b * logres + c * pow(logres, 3));
    if (in_kelvin) return tK;
    return tK - 273.15;
}

float Itkpixv2Cfg::readMosTemp(float deltaV, TransSensor sensor, bool in_kelvin) const
{
    // Convert voltage difference into temperature from https://indico.cern.ch/event/1011941/contributions/4278988/attachments/2210633/3741190/ITKPIXV2_calibatrion_sensor_temperature.pdf
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
