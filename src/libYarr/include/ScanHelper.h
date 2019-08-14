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



#include "Bookkeeper.h"
#include "HwController.h"
#include "FrontEnd.h"

#include "AllHwControllers.h"
#include "AllChips.h"

#include "storage.hpp"

namespace ScanHelper {
        json openJsonFile(std::string filepath);
        std::unique_ptr<HwController> loadController(json &ctrlCfg);
        std::string loadChips(json &j, Bookkeeper &bookie, HwController *hwCtrl, std::map<FrontEnd*, std::string> &feCfgMap, std::string &outputDir);
}
#endif
