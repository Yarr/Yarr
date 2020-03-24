// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: FE-I4 Library
// # Comment: FEI4 Config Base class
// ################################

#include "Fei4Cfg.h"

unsigned Fei4Cfg::getChipId() {
	return chipId;
}

void Fei4Cfg::setChipId(unsigned arg_chipId) {
	chipId = arg_chipId;
}


void Fei4Cfg::toFileJson(json &j) {
    j["FE-I4B"]["name"] = name;

    j["FE-I4B"]["Parameter"]["chipId"] = chipId;
    j["FE-I4B"]["Parameter"]["sCap"] = sCap;
    j["FE-I4B"]["Parameter"]["lCap"] = lCap;
    j["FE-I4B"]["Parameter"]["vcalOffset"] = vcalOffset;
    j["FE-I4B"]["Parameter"]["vcalSlope"] = vcalSlope;

    Fei4PixelCfg::toFileJson(j);
    Fei4GlobalCfg::toFileJson(j);
}

void Fei4Cfg::fromFileJson(json &j) {
    if (!j["FE-I4B"]["name"].empty())
        name = j["FE-I4B"]["name"];

    if (!j["FE-I4B"]["Parameter"]["chipId"].empty())
        chipId = j["FE-I4B"]["Parameter"]["chipId"];
    if (!j["FE-I4B"]["Parameter"]["sCap"].empty())
        sCap = j["FE-I4B"]["Parameter"]["sCap"];
    if (!j["FE-I4B"]["Parameter"]["lCap"].empty())
        lCap = j["FE-I4B"]["Parameter"]["lCap"];
    if (!j["FE-I4B"]["Parameter"]["vcalOffset"].empty())
        vcalOffset = j["FE-I4B"]["Parameter"]["vcalOffset"];
    if (!j["FE-I4B"]["Parameter"]["vcalSlope"].empty())
        vcalSlope = j["FE-I4B"]["Parameter"]["vcalSlope"];

    Fei4PixelCfg::fromFileJson(j);
    Fei4GlobalCfg::fromFileJson(j);
}
