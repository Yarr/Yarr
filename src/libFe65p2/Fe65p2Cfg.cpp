#include "Fe65p2Cfg.h"

void Fe65p2Cfg::writeConfig(json &j) {
    j["FE65-P2"]["name"] = name;
    j["FE65-P2"]["txChannel"] = txChannel;
    j["FE65-P2"]["rxChannel"] = rxChannel;
    
    j["FE65-P2"]["Parameter"]["cap"] = cap;
    j["FE65-P2"]["Parameter"]["vcalSlope"] = vcal_slope;
    j["FE65-P2"]["Parameter"]["vcalOffset"] = vcal_offset;

    Fe65p2GlobalCfg::writeConfig(j);
    Fe65p2PixelCfg::writeConfig(j);
}

void Fe65p2Cfg::loadConfig(const json &j) {
    if (j.contains({"FE65-P2","name"}))
        name = j["FE65-P2"]["name"];
    if (j.contains({"FE65-P2","txChannel"}))
        txChannel = j["FE65-P2"]["txChannel"];
    if (j.contains({"FE65-P2","rxChannel"}))
        rxChannel = j["FE65-P2"]["rxChannel"];

    if (j.contains({"FE65-P2","Parameter","cap"}))
        cap = j["FE65-P2"]["Parameter"]["cap"];
    if (j.contains({"FE65-P2","Parameter","vcalSlope"}))
        vcal_slope = j["FE65-P2"]["Parameter"]["vcalSlope"];
    if (j.contains({"FE65-P2","Parameter","vcalOffset"}))
        vcal_offset = j["FE65-P2"]["Parameter"]["vcalOffset"];

    Fe65p2GlobalCfg::loadConfig(j);
    Fe65p2PixelCfg::loadConfig(j);
}
