//
// Created by wittgen on 3/28/22.
//

#ifndef YARR_SCANCONSOLEIMPL_H
#define YARR_SCANCONSOLEIMPL_H

#include <string>
#include <vector>
#include <map>

#include "ScanOpts.h"
#include "ScanBase.h"
#include "HwController.h"
#include "Bookkeeper.h"
#include "FeedbackBase.h"
#include "DataProcessor.h"
#include "DBHandler.h"
#include "YarrDiagram.h"

#include "storage.hpp"
#include "logging.h"

class ScanConsoleImpl {
public:
    ScanConsoleImpl();
    static std::string parseConfig(const std::vector<std::string> &args);
    int init(ScanOpts options);
    int init(int argc, char *argv[]);
    int init(const std::vector<std::string> &args);
    int loadConfig();
    static int loadConfig(const json &config) {return 0;};
    int loadConfig(const char *config);
    unsigned getRunNumber();
    int setupScan();
    int configure();
    void plot();
    int initHardware();
    void cleanup();
    std::string getResults();
    void getResults(json &result);
    static std::vector<std::string> getLog(std::size_t lim) {return logging::getLog(lim);}
    void run();
    void dump();
    static void setupLogger(const char *config = nullptr);
    ~ScanConsoleImpl() = default;

private:
    ScanOpts scanOpts;
    unsigned runCounter{};
    std::string strippedScan{};
    std::string dataDir{};
    json scanLog;
    std::unique_ptr<HwController> hwCtrl{};
    std::unique_ptr<Bookkeeper> bookie{};
    std::map<unsigned, std::array<std::string, 2>> feCfgMap;
    std::unique_ptr<ScanBase> scanBase{};
    std::map<unsigned, std::unique_ptr<DataProcessor> > procs{};
    std::map<unsigned, std::unique_ptr<DataProcessor> > histogrammers{};
    std::map<unsigned, std::vector<std::unique_ptr<DataProcessor>> > analyses{};
    FeedbackClipboardMap fbData;
    std::string chipType{};
    std::string timestampStr{};
    std::time_t now{};
    json loggerConfig;
    json chipConfig;
    json dbCfg;
    json ctrlCfg;
    json userCfg;
    json siteCfg;
    json scanCfg;
    json scanConsoleConfig;
    std::unique_ptr<DBHandler> database;
    std::chrono::steady_clock::time_point cfg_end, cfg_start;
    std::chrono::steady_clock::time_point scan_done, scan_start;
    std::chrono::steady_clock::time_point processor_done, all_done;
    YarrDiagram diagram;
};


#endif //YARR_SCANCONSOLEIMPL_H
