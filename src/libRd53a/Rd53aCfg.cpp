// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: RD53A config base class
// # Date: Jun 2017
// ################################

#include "Rd53aCfg.h"

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
        j["RD53A"]["Parameter"]["Name"] = name;
    if (!j["RD53A"]["Parameter"]["ChipId"].empty())
        j["RD53A"]["Parameter"]["ChipId"] = m_chipId;
    if (!j["RD53A"]["Parameter"]["InjCap"].empty())
        j["RD53A"]["Parameter"]["InjCap"] = m_injCap;
    if (!j["RD53A"]["Parameter"]["VcalPar"].empty())
        j["RD53A"]["Parameter"]["VcalPar"] = m_vcalPar;
    Rd53aGlobalCfg::fromFileJson(j);
    Rd53aPixelCfg::fromFileJson(j);
}
