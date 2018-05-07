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
