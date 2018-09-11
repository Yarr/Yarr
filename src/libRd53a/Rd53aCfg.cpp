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
    , m_vcalPar ( {{ -1.0, 0.195, 0.0, 0.0 }} )
{}

double Rd53aCfg::toCharge(double vcal) {
    // Q = C*V
    // Linear is good enough
    double V = (m_vcalPar[0]*Unit::Milli + m_vcalPar[1]*vcal*Unit::Milli)/Physics::ElectronCharge;
    return V*m_injCap*Unit::Femto;
}

double Rd53aCfg::toCharge(double vcal, bool sCap, bool lCap) { return toCharge(vcal); }

unsigned Rd53aCfg::toVcal(double charge) {
    double V= (charge*Physics::ElectronCharge)/(m_injCap*Unit::Femto);
    unsigned vcal = (unsigned) round((V)/(m_vcalPar[1]*Unit::Milli)); // Note: no offset applied
    return vcal;
}

void Rd53aCfg::setChipId(unsigned id) {
    m_chipId = id;
}

void Rd53aCfg::toFileJson(json &j) {
    j["RD53A"]["Parameter"]["Name"] = name;
    j["RD53A"]["Parameter"]["ChipId"] = m_chipId;
    j["RD53A"]["Parameter"]["InjCap"] = m_injCap;
    j["RD53A"]["Parameter"]["VcalPar"] = m_vcalPar;

    Rd53aGlobalCfg::toFileJson(j);
    Rd53aPixelCfg::toFileJson(j);
}

void Rd53aCfg::fromFileJson(json &j) {
    if (!j["RD53A"]["Parameter"]["Name"].empty())
        name = j["RD53A"]["Parameter"]["Name"];
    if (!j["RD53A"]["Parameter"]["ChipId"].empty())
        m_chipId = j["RD53A"]["Parameter"]["ChipId"];
    if (!j["RD53A"]["Parameter"]["InjCap"].empty())
        m_injCap = j["RD53A"]["Parameter"]["InjCap"];
    if (!j["RD53A"]["Parameter"]["VcalPar"].empty())
        m_vcalPar = j["RD53A"]["Parameter"]["VcalPar"];
    Rd53aGlobalCfg::fromFileJson(j);
    Rd53aPixelCfg::fromFileJson(j);
}
