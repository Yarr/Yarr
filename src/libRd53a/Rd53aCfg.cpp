// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A config base class
// # Date: Jun 2017
// ################################

#include "Rd53aCfg.h"

    Rd53aCfg::Rd53aCfg()
    : m_chipId  ( 0 )
    , m_injCap  ( 8.2 )
    , m_vcalPar ( {{ -1.0, 0.215, 0.0, 0.0 }} )
    , m_ADCcalPar ( {{ 10.53, 0.1932}} )
    , m_TempSenPar ( {{
        {{-278.0, 3502.8}},
        {{-270.3, 3414.7}},	                 
        {{-271.2, 3403.0}},
        {{-267.4, 3363.1}} }} )
    , m_RadSenPar ( {{
        {{-259.5, 4086.4}},
        {{-273.3, 4271.0}},
        {{-263.0, 4139.3}},
        {{-271.1, 4212.4}} }} )
{}

double Rd53aCfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
    double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
    return V*m_injCap*Unit::Femto;
}

void Rd53aCfg::enableAll() {
    // logger->info("Resetting enable/hitbus pixel mask to all enabled!");
    for (unsigned int col = 0; col < n_Col; col++) {
        for (unsigned row = 0; row < n_Row; row ++) {
            setEn(col, row, 1);
            setHitbus(col, row, 1);
        }
    }
}


double Rd53aCfg::toCharge(double vcal, bool sCap, bool lCap) { return toCharge(vcal); }

unsigned Rd53aCfg::toVcal(double charge) {
    double V= (charge*Physics::ElectronCharge)/(m_injCap*Unit::Femto);
    unsigned vcal = (unsigned) round((V)/(m_vcalPar[1]*Unit::Milli)); // Note: no offset applied
    return vcal;
}

float Rd53aCfg::ADCtoV(uint16_t ADC) {
    return (float(ADC)*m_ADCcalPar[1]+m_ADCcalPar[0])*Unit::Milli;
}

float Rd53aCfg::VtoTemp(float V, uint16_t Sensor=1, bool isRadSensor=false) {
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

void Rd53aCfg::setChipId(unsigned id) {
    m_chipId = id;
}

unsigned Rd53aCfg::getChipId() const {
    return m_chipId;
}

void Rd53aCfg::writeConfig(json &j) {
    j["RD53A"]["Parameter"]["Name"] = name;
    j["RD53A"]["Parameter"]["ChipId"] = m_chipId;
    j["RD53A"]["Parameter"]["InjCap"] = m_injCap;
    for(unsigned  i=0;i<4;i++)  j["RD53A"]["Parameter"]["VcalPar"][i]= m_vcalPar[i];
    for(unsigned  i=0;i<2;i++)  j["RD53A"]["Parameter"]["ADCcalPar"][i]= m_ADCcalPar[i];

    for(unsigned  sens=0;sens<4;sens++) {
        for(unsigned  i=0;i<2;i++)  j["RD53A"]["Parameter"]["TempSen"+std::to_string(sens)+"Par"][i] = m_TempSenPar[sens][i];
        for(unsigned  i=0;i<2;i++)  j["RD53A"]["Parameter"]["RadSen"+std::to_string(sens)+"Par"][i] = m_RadSenPar[sens][i];
    }

    Rd53aGlobalCfg::writeConfig(j);
    Rd53aPixelCfg::writeConfig(j);
}

void Rd53aCfg::loadConfig(const json &j) {
    if (j.contains({"RD53A","Parameter","Name"}))
        name = j["RD53A"]["Parameter"]["Name"];
    if (j.contains({"RD53A","Parameter","ChipId"}))
        m_chipId = j["RD53A"]["Parameter"]["ChipId"];
    if (j.contains({"RD53A","Parameter","InjCap"}))
        m_injCap = j["RD53A"]["Parameter"]["InjCap"];
    if (j.contains({"RD53A","Parameter","VcalPar"}))
        for(unsigned  i=0;i<4;i++)  m_vcalPar[i] = j["RD53A"]["Parameter"]["VcalPar"][i];

    if (j.contains({"RD53A","Parameter","ADCcalPar"}))
        for(unsigned  i=0;i<2;i++)  m_ADCcalPar[i] = j["RD53A"]["Parameter"]["ADCcalPar"][i];

    for(unsigned  sens=0;sens<4;sens++) {
        if (j.contains({"RD53A","Parameter","TempSen"+std::to_string(sens)+"Par"}))
            for(unsigned  i=0;i<2;i++)  m_TempSenPar[sens][i] = j["RD53A"]["Parameter"]["TempSen"+std::to_string(sens)+"Par"][i];
        if (j.contains({"RD53A","Parameter","RadSen"+std::to_string(sens)+"Par"}))
            for(unsigned  i=0;i<2;i++)  m_RadSenPar[sens][i] = j["RD53A"]["Parameter"]["RadSen"+std::to_string(sens)+"Par"][i];
    }

    Rd53aGlobalCfg::loadConfig(j);
    Rd53aPixelCfg::loadConfig(j);
}
