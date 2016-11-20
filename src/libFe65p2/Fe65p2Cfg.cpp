#include "Fe65p2Cfg.h"

void Fe65p2Cfg::toFileJson(json &j) {
    j["FE65-P2"]["name"] = name;
    j["FE65-P2"]["txChannel"] = txChannel;
    j["FE65-P2"]["rxChannel"] = rxChannel;
    
    j["FE65-P2"]["Parameter"]["cap"] = cap;
    j["FE65-P2"]["Parameter"]["vcalSlope"] = vcal_slope;
    j["FE65-P2"]["Parameter"]["vcalOffset"] = vcal_offset;

    Fe65p2GlobalCfg::toFileJson(j);
    Fe65p2PixelCfg::toFileJson(j);
}

void Fe65p2Cfg::fromFileJson(json &j) {
    name = j["FE65-P2"]["name"];
    txChannel = j["FE65-P2"]["txChannel"];
    rxChannel = j["FE65-P2"]["rxChannel"];
    
    cap = j["FE65-P2"]["Parameter"]["cap"];
    vcal_slope = j["FE65-P2"]["Parameter"]["vcalSlope"];
    vcal_offset = j["FE65-P2"]["Parameter"]["vcalOffset"];

    Fe65p2GlobalCfg::fromFileJson(j);
    Fe65p2PixelCfg::fromFileJson(j);

}
