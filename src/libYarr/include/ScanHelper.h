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

#include "AnalysisDataProcessor.h"
#include "Bookkeeper.h"
#include "FeDataProcessor.h"
#include "FeedbackBase.h"
#include "FrontEnd.h"
#include "HistoDataProcessor.h"
#include "HwController.h"
#include "ScanLoopInfo.h"
#include "Utils.h"

#include "storage.hpp"
#include "logging.h"

#include "ScanOpts.h"
#include "ScanBase.h"

namespace ScanHelper {
        /// Get a new run number, such that it's different next time
        unsigned newRunCounter();

        json openJsonFile(const std::string& filepath);
        std::unique_ptr<HwController> loadController(const json &ctrlCfg);
        std::string buildChips(const json &j, Bookkeeper &bookie, HwController *hwCtrl, std::map<unsigned,
                              std::array<std::string, 2>> &feCfgMap);
        
        std::string loadChipConfigs(json &j, const bool &createConfig, const std::string &dir);
        std::string loadChipConfigs(json &j, bool createConfig=false);
        int loadConfigFile(const ScanOpts &scanOpts, bool writeConfig, json &config);
// TODO Do not want to use the raw pointer ScanBase*
        void buildHistogrammers( std::map<unsigned, std::unique_ptr<HistoDataProcessor>>& histogrammers, const json &scanConfig,
                                 Bookkeeper &bookie, std::string outputDir);

// TODO would prefer not to need bookie --> deep dependency!
// TODO Do not want to use the raw pointer ScanBase*
        void buildRawDataProcs( std::map<unsigned, std::unique_ptr<FeDataProcessor> > &procs,
                           Bookkeeper &bookie,
                           const std::string &chipType);
        void buildAnalyses( std::map<unsigned, std::vector<std::unique_ptr<AnalysisDataProcessor>> >& analyses,
                            const json& scanType, Bookkeeper& bookie, const ScanLoopInfo* s, FeedbackClipboardMap *fbMap, int mask_opt, std::string outputDir,
                            int target_tot, int target_charge);
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
       void writeScanLog(json scanLog, const std::string &filename);
       void writeFeConfig(FrontEndCfg *feCfg, const std::string &filename);
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
