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
        // A 2D vector of int to store algorithm indices for all tiers of analyses
        using AlgoTieredIndex = std::vector<std::vector<int>>;

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

        /// Setup analysis for one front end
        /**
           @param analyses Vector of data processors for analysis tiers.
           @param feCfg FrontEndCfg object to be modified by analysis.
           @param anaCfg Configuration of analysis algorithms.
           @param geo Geometry for output histograms.
           @param algoIndexTiers Dependency information between tiers.
           @param fbData Feedback connection to scan engine.
           @param clipResults ClipBoard for histograms between tiers.
           @param clipHisto ClipBoard for input histograms.
           @param scanInfo Information about scan loops.
           @param mask_opt Do mask flag.
           @param outputDir Directory for HistogramArchiver (skip if empty).
           @param target_tot ToT target.
           @param target_charge Charge target.
         */
        void buildAnalysisForFrontEnd(std::vector<std::unique_ptr<AnalysisDataProcessor>> &analyses,
                                  unsigned feId,
                                  FrontEndCfg *feCfg,
                                  const json &anaCfg,
                                  FrontEndGeometry &geo,
                                  const AlgoTieredIndex &algoIndexTiers,
                                  FeedbackClipboard *fbData,
                                  std::vector<std::unique_ptr<ClipBoard<HistogramBase>>> &clipResults,
                                  ClipBoard<HistogramBase> &clipHisto,
                                  const ScanLoopInfo *scanInfo,
                                  int mask_opt,
                                  const std::string &outputDir,
                                  int target_tot, int target_charge);

        void buildAnalyses( std::map<unsigned, std::vector<std::unique_ptr<AnalysisDataProcessor>> >& analyses,
                            const json& scanType, Bookkeeper& bookie, const ScanLoopInfo* s, FeedbackClipboardMap *fbMap, int mask_opt, std::string outputDir,
                            int target_tot, int target_charge);
        void buildAnalysisHierarchy(AlgoTieredIndex& indexTiers,
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
