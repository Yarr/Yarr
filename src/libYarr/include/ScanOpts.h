//
// Created by wittgen on 1/31/22.
//
#ifndef YARR_SCANOPTS_H
#define YARR_SCANOPTS_H
#include <vector>
#include <string>

struct ScanOpts {
    std::string defaultLogPattern = "[%T:%e]%^[%=8l][%=15n][%t]:%$ %v";
    bool scan_config_provided = false;
    std::string scanType;
    std::vector<std::string> cConfigPaths;
    std::string outputDir = "./data/";
    std::string ctrlCfgPath;
    bool doPlots = false;
    int target_charge{-1};
    int target_tot{-1};
    int mask_opt{-1};
    bool dbUse = false;
    bool doOutput = true;
    std::string logCfgPath;
    std::string dbCfgPath ;
    std::string dbSiteCfgPath;
    std::string dbUserCfgPath;
    bool setQCMode = false;
    bool setInteractiveMode = false;
    std::string commandLineStr;
    std::string progName;
    bool doResetBeforeScan = true;
    bool makeGraph = false;
};
#endif //YARR_SCANOPTS_H
