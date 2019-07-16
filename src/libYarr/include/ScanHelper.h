#ifndef SCANHELPER_H
#define SCANHELPER_H

// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Class managing scans
// # Comment: Old scanConsole in a class
// ################################

#include <string>

#include "json.hpp"

#include "Bookkeeper.h"
#include "HwController.h"
#include "FrontEnd.h"

#include "AllHwControllers.h"
#include "AllChips.h"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

namespace ScanHelper {
        json openJsonFile(std::string filepath);
        std::unique_ptr<HwController> loadController(json &ctrlCfg);
        std::string loadChips(json &j, Bookkeeper &bookie, HwController *hwCtrl, std::map<FrontEnd*, std::string> &feCfgMap, std::string &outputDir);
}
#endif
