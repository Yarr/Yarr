// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Config Base class
// ################################

#include "Fei4Cfg.h"

unsigned Fei4Cfg::getChipId() const {
	return chipId;
}

void Fei4Cfg::setChipId(unsigned arg_chipId) {
	chipId = arg_chipId;
}


void Fei4Cfg::writeConfig(json &j) {
    j["FE-I4B"]["name"] = name;

    j["FE-I4B"]["Parameter"]["chipId"] = chipId;
    j["FE-I4B"]["Parameter"]["sCap"] = sCap;
    j["FE-I4B"]["Parameter"]["lCap"] = lCap;
    j["FE-I4B"]["Parameter"]["vcalOffset"] = vcalOffset;
    j["FE-I4B"]["Parameter"]["vcalSlope"] = vcalSlope;

    Fei4PixelCfg::writeConfig(j);
    Fei4GlobalCfg::writeConfig(j);
}

void Fei4Cfg::enableAll() {
    for (unsigned int dc = 0; dc < n_DC; dc++) {
        En(dc).setAll(1);
        Hitbus(dc).setAll(0);
    }
}

void Fei4Cfg::loadConfig(const json &j) {
    if (j.contains("FE-I4B") && j["FE-I4B"].contains("name"))
        name = j["FE-I4B"]["name"];

    if (j.contains("FE-I4B") && j["FE-I4B"].contains("Parameter")) {
        auto &jparams = j["FE-I4B"]["Parameter"];
        if (jparams.contains("chipId"))
            chipId = jparams["chipId"];
        if (jparams.contains("sCap"))
            sCap = jparams["sCap"];
        if (jparams.contains("lCap"))
            lCap = jparams["lCap"];
        if (jparams.contains("vcalOffset"))
            vcalOffset = jparams["vcalOffset"];
        if (jparams.contains("vcalSlope"))
            vcalSlope = jparams["vcalSlope"];
    }

    Fei4PixelCfg::loadConfig(j);
    Fei4GlobalCfg::loadConfig(j);
}
