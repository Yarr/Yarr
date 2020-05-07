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
    m_chipId(0),
    m_vcalPar({{-1.0, 0.215}})
{}

double Rd53bCfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
    double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
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

void Rd53bCfg::toFileJson(json &j) {
    // General Paramters
    j["RD53B"]["Parameter"]["Name"] = name;
    j["RD53B"]["Parameter"]["ChipId"] = m_chipId;
    j["RD53B"]["Parameter"]["InjCap"] = m_injCap;
    for(unsigned  i=0;i<m_vcalPar.size();i++)  
        j["RD53B"]["Parameter"]["VcalPar"][i]= m_vcalPar[i];
}

void Rd53bCfg::fromFileJson(json &j) {

}

void Rd53bCfg::setChipId(unsigned id) {
    m_chipId = id;
}
