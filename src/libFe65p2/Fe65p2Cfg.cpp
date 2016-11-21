#include "Fe65p2Cfg.h"

void Fe65p2Cfg::toFileJson(json &j) {
    j["FE65-P2"]["Parameter"]["name"] = name;
    j["FE65-P2"]["Parameter"]["cap"] = cap;
    j["FE65-P2"]["Parameter"]["vcalSlope"] = vcal_slope;
    j["FE65-P2"]["Parameter"]["vcalOffset"] = vcal_offset;
    j["FE65-P2"]["Parameter"]["txChannel"] = this->txChannel;
    j["FE65-P2"]["Parameter"]["rxChannel"] = this->rxChannel;

    Fe65p2GlobalCfg::toFileJson(j);
    Fe65p2PixelCfg::toFileJson(j);
}

void Fe65p2Cfg::fromFileJson(json &j) {
    name = j["FE65-P2"]["Parameter"]["name"];
    cap = j["FE65-P2"]["Parameter"]["cap"];
    vcal_slope = j["FE65-P2"]["Parameter"]["vcalSlope"];
    vcal_offset = j["FE65-P2"]["Parameter"]["vcalOffset"];
    this->txChannel = j["FE65-P2"]["Parameter"]["txChannel"];
    this->rxChannel = j["FE65-P2"]["Parameter"]["rxChannel"];

    Fe65p2GlobalCfg::fromFileJson(j);
    Fe65p2PixelCfg::fromFileJson(j);
}
