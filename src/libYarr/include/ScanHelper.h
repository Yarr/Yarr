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
#include "FeedbackBase.h"
#include "FrontEnd.h"
#include "HwController.h"

#include "storage.hpp"
#include "logging.h"

#include "ScanOpts.h"

namespace ScanHelper {
        /// Get a new run number, such that it's different next time
        unsigned newRunCounter();

        json openJsonFile(const std::string& filepath);
        std::unique_ptr<HwController> loadController(const json &ctrlCfg);
        std::string loadChips(const json &j, Bookkeeper &bookie, HwController *hwCtrl, std::map<FrontEnd*,
                              std::array<std::string, 2>> &feCfgMap,const std::string &outputDir);
        std::string loadChips(const json &j, Bookkeeper &bookie, HwController *hwCtrl, std::map<FrontEnd*,
                              std::array<std::string, 2>> &feCfgMap);
        std::string loadChipConfigs(json &j, bool createConfig=false);
        void writeConfigFiles(const json &config, const std::string &outputDir);
        int loadConfigs(const ScanOpts &scanOpts, bool writeConfig, json &config);
// TODO Do not want to use the raw pointer ScanBase*
        void buildHistogrammers( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& histogrammers, const json &scanConfig, std::vector<FrontEnd*>& feList, ScanBase* s, std::string outputDir);

// TODO would prefer not to need bookie --> deep dependency!
// TODO Do not want to use the raw pointer ScanBase*
        void buildAnalyses( std::map<FrontEnd*, std::vector<std::unique_ptr<DataProcessor>> >& analyses,
                            const json& scanType, Bookkeeper& bookie, ScanBase* s, FeedbackClipboardMap *fbMap, int mask_opt);
        void buildAnalysisHierarchy(std::vector<std::vector<int>>& indexTiers,
                                    const json &anaCfg);
        template <typename T>
            std::string toString(T value,int digitsCount);
        std::unique_ptr<ScanBase> buildScan( const json& scanType,
                                             Bookkeeper& bookie,
                                             FeedbackClipboardMap *fbData);
       std::string defaultDbUserCfgPath();
       std::string defaultDbSiteCfgPath();
       std::string defaultDbCfgPath();
       std::string getHostname();
       std::string defaultDbDirPath();
       std::string createOutputDir(const std::string &scanType, unsigned int runCounter, std::string &outputDir);
       void createSymlink(const std::string &dataDir, const std::string &strippedScan, unsigned int runCounter);
       std::string timestamp(std::time_t);
       void banner(std::shared_ptr<spdlog::logger> &logger, const std::string &msg);
       void listChips();
       void listProcessors();
       void listScans();
       void listControllers();
       void listScanLoopActions();
       void listKnown();
       bool lsdir(const std::string &dataDir);
       void printHelp();
       int parseOptions(int argc, char *argv[], ScanOpts &scanOpts);
}

#endif
