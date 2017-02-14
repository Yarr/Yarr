#include "EmuController.h"

void EmuController::loadConfig(nlohmann::json &j) {
    EmuTxCore::setCom(new EmuShm(j["tx"]["id"], j["tx"]["size"], true));
    EmuRxCore::setCom(new EmuShm(j["rx"]["id"], j["rx"]["size"], true));
}
