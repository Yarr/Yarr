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
#include "DataProcessor.h"
#include "FrontEnd.h"
#include "HwController.h"

#include "storage.hpp"

namespace ScanHelper {
        /// Get a new run number, such that it's different next time
        unsigned newRunCounter();

        json openJsonFile(std::string filepath);
        std::unique_ptr<HwController> loadController(json &ctrlCfg);
        std::string loadChips(json &j, Bookkeeper &bookie, HwController *hwCtrl, std::map<FrontEnd*, std::string> &feCfgMap, std::string &outputDir);

// TODO Do not want to use the raw pointer ScanBase*
        void buildHistogrammers( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& histogrammers, const std::string& scanType, std::vector<FrontEnd*>& feList, ScanBase* s, std::string outputDir);

// TODO would prefer not to need bookie --> deep dependency!
// TODO Do not want to use the raw pointer ScanBase*
        void buildAnalyses( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& analyses,
                            const std::string& scanType, Bookkeeper& bookie, ScanBase* s, int mask_opt);
}

#endif
