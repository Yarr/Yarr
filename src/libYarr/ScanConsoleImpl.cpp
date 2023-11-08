
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <map>

#include "logging.h"
#include "LoggingConfig.h"

#include "ScanHelper.h"
#include "ScanOpts.h"

#include "HwController.h"
#include "AllChips.h"
#include "AllProcessors.h"
#include "AllStdActions.h"
#include "Bookkeeper.h"
#include "FeedbackBase.h"
#include "ScanBase.h"
#include "DBHandler.h"

#include "storage.hpp"
#include "ScanConsoleImpl.h"

#include "yarr.h"

auto logger = logging::make_log("ScanConsole");

ScanConsoleImpl::ScanConsoleImpl() = default;

std::string ScanConsoleImpl::parseConfig(const std::vector<std::string> &args) {
    json result;
    result["status"] = "failed";
    int argc = args.size();
    char *argv[argc+1];
    for (int i = 0; i < argc; i++) {
        argv[i] = (char *) args[i].c_str();
    }
    argv[argc] = nullptr; // should be a null terminated array
    ScanOpts options;
    json scanConsoleConfig;
    int res = ScanHelper::parseOptions(argc, argv, options);
    if (res==1) {
        res = ScanHelper::loadConfigFile(options, false, scanConsoleConfig);
        if (res>=0) {
            result["status"] = "ok";
        }
    }
    std::string str;
    result["config"] = scanConsoleConfig;
    result["runCounter"] = ScanHelper::newRunCounter();
    result.dump(str);
    return str;
}

int ScanConsoleImpl::init(int argc, char *argv[]) {
    ScanOpts options;
    int res=ScanHelper::parseOptions(argc,argv,options);
    if(res<=0) return res;
    return init(options);
}

int ScanConsoleImpl::init(const std::vector<std::string> &args) {
    int argc =  args.size();
    char *argv[argc];
    for(int i = 0;i<argc; i++) {
        argv[i] = (char *) args[i].c_str();
    }
    ScanOpts options;
    int res=ScanHelper::parseOptions(argc,argv,options);
    if(res<=0) return res;
    return init(options);
}

int ScanConsoleImpl::init(ScanOpts options) {
    scanOpts=std::move(options);
    runCounter = ScanHelper::newRunCounter();
    if(!scanOpts.logCfgPath.empty()) {
        loggerConfig = ScanHelper::openJsonFile(scanOpts.logCfgPath);
        loggerConfig["outputDir"]=scanOpts.outputDir;
    } else {
        // default log setting
        loggerConfig["pattern"] = scanOpts.defaultLogPattern;
        loggerConfig["log_config"][0]["name"] = "all";
        loggerConfig["log_config"][0]["level"] = "info";
        loggerConfig["outputDir"]="";
    }
    spdlog::info("Configuring logger ...");
    logging::setupLoggers(loggerConfig);
    ScanHelper::banner(logger,"Welcome to YARR - ScanConsole");
    return 1;
}


int ScanConsoleImpl::loadConfig() {
    int result = ScanHelper::loadConfigFile(scanOpts, true, scanConsoleConfig);
    if(result<0) {
        logger->error("Failed to read configs");
        return -1;
    }
    ctrlCfg=scanConsoleConfig["ctrlConfig"];
    chipConfig=scanConsoleConfig["chipConfig"];
    scanCfg=scanConsoleConfig["scanCfg"];
    if(scanOpts.dbUse) {
        try {
            dbCfg = ScanHelper::openJsonFile(scanOpts.dbCfgPath);
            userCfg = ScanHelper::openJsonFile(scanOpts.dbUserCfgPath);
            siteCfg = ScanHelper::openJsonFile(scanOpts.dbSiteCfgPath);
        }catch (std::runtime_error &e) {
            logger->critical("#ERROR# failed to load database config: {}", e.what());
            return -1;
        }
    }
    dataDir=scanOpts.outputDir;
    if(scanOpts.doOutput) {
        strippedScan = ScanHelper::createOutputDir(scanOpts.scanType, runCounter, scanOpts.outputDir);
        ScanHelper::createSymlink(dataDir,strippedScan,runCounter);
    }

    if(scanOpts.scan_config_provided) {
        logger->info("Scan Type/Config {}", scanOpts.scanType);
    } else {
        logger->info("No scan configuration provided, will only configure front-ends");
    }
    logger->info("Connectivity:");
    for(std::string const& sTmp : scanOpts.cConfigPaths){
        logger->info("    {}", sTmp);
    }
    logger->info("Target ToT: {}", scanOpts.target_tot);
    logger->info("Target Charge: {}", scanOpts.target_charge);
    logger->info("Output Plots: {}", scanOpts.doPlots);
    logger->info("Output Directory: {}", scanOpts.outputDir);
    return 0;
}

// load scan config from a JSON string
int ScanConsoleImpl::loadConfig(const char *config){
    loggerConfig["pattern"] = scanOpts.defaultLogPattern;
    loggerConfig["log_config"][0]["name"] = "all";
    loggerConfig["log_config"][0]["level"] = "info";
    loggerConfig["outputDir"]="";
    spdlog::info("Configuring logger ...");
    logging::setupLoggers(loggerConfig);
    json j;
    json::parse(config,j);
    json scanConsoleConfig = j["config"];
    runCounter=j["runCounter"];
    ctrlCfg=scanConsoleConfig["ctrlConfig"];
    chipConfig=scanConsoleConfig["chipConfig"];
    scanCfg=scanConsoleConfig["scanCfg"];
    scanOpts.doOutput=false;
    scanOpts.scan_config_provided=true;
    return 0;
}

unsigned ScanConsoleImpl::getRunNumber() {
    return runCounter;
}


int ScanConsoleImpl::setupScan() {
    ScanHelper::banner(logger,"Setup Scan");

    // Make backup of scan config

    // Create backup of current config
    if (scanOpts.doOutput &&
        scanOpts.scanType.find("json") != std::string::npos) {
        // TODO fix folder
        std::ifstream cfgFile(scanOpts.scanType);
        std::ofstream backupCfgFile(scanOpts.outputDir + strippedScan + ".json");
        backupCfgFile << cfgFile.rdbuf();
        backupCfgFile.close();
        cfgFile.close();
    }

    // TODO Make this nice
    try {
        scanBase = ScanHelper::buildScan(scanCfg, *bookie, &fbData);
    } catch (const char *msg) {
        logger->warn("No scan to run, exiting with msg: {}", msg);
        return 0;
    }
    // TODO not to use the raw pointer!
    try {
        ScanHelper::buildRawDataProcs(procs, *bookie, chipType);
        ScanHelper::buildHistogrammers(histogrammers, scanCfg, *bookie, scanBase.get(), scanOpts.outputDir);
        ScanHelper::buildAnalyses(analyses, scanCfg, *bookie, scanBase.get(),
                                  &fbData, scanOpts.mask_opt, scanOpts.outputDir);
    } catch (const char *msg) {
        logger->error("{}", msg);
        return -1;
    }

    // Make YARR diagram
    if (scanOpts.makeGraph) {
        diagram.makeDiagram(*bookie, procs, histogrammers, analyses);
    }

    logger->info("Running pre scan!");
    scanBase->init();
    scanBase->preScan();

    // Run from downstream to upstream
    logger->info("Starting histogrammer and analysis threads:");
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        if (!fe->isActive()) continue;
        for (auto& ana : analyses[id]) {
            ana->init();
            ana->run();
        }
        histogrammers[id]->init();
        histogrammers[id]->run();
        
        procs[id]->init();
        procs[id]->run();
     
        logger->info(" .. started threads of Fe {}", id);
    }

    return 0;
}

void ScanConsoleImpl::plot() {
 if(scanOpts.doPlots) {
        bool ok = ScanHelper::lsdir(dataDir+ "last_scan/");
        if(!ok)
            logger->info("Find plots in: {}last_scan", dataDir);
 }

}

int ScanConsoleImpl::configure() {
    // Initial setting local DBHandler
    if (scanOpts.dbUse) {
        database = std::make_unique<DBHandler>();
        ScanHelper::banner(logger,"Set Database");
        database->initialize(scanOpts.dbCfgPath, scanOpts.progName, scanOpts.setQCMode, scanOpts.setInteractiveMode);
        if (database->checkConfigs(scanOpts.dbUserCfgPath, scanOpts.dbSiteCfgPath, scanOpts.cConfigPaths)==1)
            return -1;
        scanLog["dbCfg"] = dbCfg;
        scanLog["userCfg"] = userCfg;
        scanLog["siteCfg"] = siteCfg;
    }

    // Reset masks
    if (scanOpts.mask_opt == 1) {
        for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
            FrontEnd *fe = bookie->getEntry(id).fe;
            auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
            feCfg->enableAll();
        }
    }
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
        if(scanOpts.doOutput)
            ScanHelper::writeFeConfig(feCfg, scanOpts.outputDir + feCfgMap.at(id)[1] + ".before");
    }
    bookie->initGlobalFe(chipType);
    bookie->getGlobalFe()->init(&*hwCtrl, 0, 0);

    ScanHelper::banner(logger,"Configure FEs");

    cfg_start = std::chrono::steady_clock::now();

    // Before configuring each FE, broadcast reset to all tx channels
    // Enable all tx channels
    hwCtrl->setCmdEnable(bookie->getTxMaskUnique());

    // send global/broadcast reset command to all frontends
    if(scanOpts.doResetBeforeScan) {
        bookie->getGlobalFe()->resetAllHard();
    }

    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
        logger->info("Configuring {}", feCfg->getName());
        // Select correct channel
        hwCtrl->setCmdEnable(feCfg->getTxChannel());
        // Configure
        fe->configure();
        // Wait for fifo to be empty
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        while(!hwCtrl->isCmdEmpty());
    }
    cfg_end = std::chrono::steady_clock::now();
    logger->info("Sent configuration to all FEs in {} ms!",
                 std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count());

    // Wait for rx to sync with FE stream
    // TODO Check RX sync
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    hwCtrl->flushBuffer();
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
        logger->info("Checking com {}", feCfg->getName());
        // Select correct channel
        hwCtrl->setCmdEnable(feCfg->getTxChannel());
        hwCtrl->setRxEnable(feCfg->getRxChannel());
        hwCtrl->checkRxSync(); // Must be done per fe (Aurora link) and after setRxEnable().
        // Configure
        if (fe->checkCom() != 1) {
            logger->critical("Can't establish communication, aborting!");
            return -1;
        }
        // check that the current FE name is valid
        if (!fe->hasValidName()) {
            logger->critical("Invalid chip name, aborting!");
            return -1;
        }

        logger->info("... success!");
    }

    // at this point, if we're not running a scan we should just exit
    if(!scanOpts.scan_config_provided) {
        return 1;
    }

    // Enable all active channels
    logger->info("Enabling Tx channels");
    hwCtrl->setCmdEnable(bookie->getTxMask());
    for (uint32_t channel : bookie->getTxMask()) {
        logger->info("Enabling Tx channel {}", channel);
    }
    logger->info("Enabling Rx channels");
    hwCtrl->setRxEnable(bookie->getRxMask());
    for (uint32_t channel : bookie->getRxMask()) {
        logger->info("Enabling Rx channel {}", channel);
    }
    return 0;
}

int ScanConsoleImpl::initHardware() {
 // Timestamp
    now = std::time(nullptr);
    timestampStr = ScanHelper::timestamp(now);
    logger->info("Timestamp: {}", timestampStr);
    logger->info("Run Number: {}", runCounter);

    // Add to scan log
    scanLog["yarr_version"] = yarr::version::get();
    scanLog["exec"] = scanOpts.commandLineStr;
    scanLog["timestamp"] = timestampStr;
    scanLog["startTime"] = (int)now;
    scanLog["runNumber"] = runCounter;
    scanLog["targetCharge"] = scanOpts.target_charge;
    scanLog["targetTot"] = scanOpts.target_tot;
    scanLog["testType"] = strippedScan;

    ScanHelper::banner(logger,"Init Hardware");

    logger->info("-> Opening controller config: {}", scanOpts.ctrlCfgPath);

    try {
        hwCtrl = ScanHelper::loadController(ctrlCfg);
    } catch (std::runtime_error &e) {
        logger->critical("Error opening or loading controller config: {}", e.what());
        return -1;
    }
    // Add to scan log
    scanLog["ctrlCfg"] = ctrlCfg;
    scanLog["ctrlStatus"] = hwCtrl->getStatus();

    hwCtrl->setupMode();

    // Disable trigger in-case
    hwCtrl->setTrigEnable(0);

    bookie=std::make_unique<Bookkeeper>(&*hwCtrl, &*hwCtrl);


    bookie->setTargetTot(scanOpts.target_tot);
    bookie->setTargetCharge(scanOpts.target_charge);

    ScanHelper::banner(logger,"Loading Configs");


    // Loop chip configs
    for(json const& config : chipConfig){
        try {
            chipType = ScanHelper::buildChips(config, *bookie, &*hwCtrl, feCfgMap);
        } catch (std::runtime_error &e) {
            logger->critical("#ERROR# loading chip config: {}", e.what());
            return -1;
        }
        scanLog["connectivity"].push_back(config);
    }


    // Initial setting local DBHandler
    if (scanOpts.dbUse) {
        database = std::make_unique<DBHandler>();
        ScanHelper::banner(logger,"Set Database");
        database->initialize(scanOpts.dbCfgPath, scanOpts.progName, scanOpts.setQCMode, scanOpts.setInteractiveMode);
        if (database->checkConfigs(scanOpts.dbUserCfgPath, scanOpts.dbSiteCfgPath, scanOpts.cConfigPaths)==1)
            return -1;
        scanLog["dbCfg"] = dbCfg;
        scanLog["userCfg"] = userCfg;
        scanLog["siteCfg"] = siteCfg;
    }

    // Reset masks
    if (scanOpts.mask_opt == 1) {
        for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
            FrontEnd *fe = bookie->getEntry(id).fe;
            auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
            feCfg->enableAll();
        }
    }
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
        if(scanOpts.doOutput)
            ScanHelper::writeFeConfig(feCfg, scanOpts.outputDir + feCfgMap.at(id)[1] + ".before");
    }
    bookie->initGlobalFe(chipType);
    bookie->getGlobalFe()->init(&*hwCtrl, 0, 0);
    return 0;
}

void ScanConsoleImpl::cleanup() {
    ScanHelper::banner(logger,"Cleanup");
    if(scanOpts.doOutput)
        ScanHelper::writeScanLog(scanLog, scanOpts.outputDir + "scanLog.json");

    // Cleanup
    //delete scanBase;
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        if(!fe->isActive()) continue;
        auto feCfg = dynamic_cast<FrontEndCfg*>(fe);

        // Save config
        if (!feCfg->isLocked() && scanOpts.doOutput) {
            const std::string &filename=feCfgMap.at(id)[0];
            logger->info("Saving config of FE {} to {}",
                         feCfg->getName(), filename);
            ScanHelper::writeFeConfig(feCfg, filename);
        } else {
            logger->warn("Not saving config for FE {} as it is protected!", feCfg->getName());
        }

        // Save extra config in data folder
        if(scanOpts.doOutput)
            ScanHelper::writeFeConfig(feCfg, scanOpts.outputDir + feCfgMap.at(id)[1] + ".after");

        // Plot
        // store output results (if any)
        if(analyses.empty()) continue;
        logger->info("-> Storing output results of FE {}", feCfg->getRxChannel());
        if (fe->clipResult.empty()) continue;
        auto &output = *(fe->clipResult.back());
        std::string name = feCfg->getName();
        if (output.empty()) {
            logger->warn(
                    "There were no results for chip {}, this usually means that the chip did not send any data at all.",
                    name);
            continue;
        }
        while(!output.empty()) {
            auto histo = output.popData();
            // only create the image files if asked to
            if(scanOpts.doPlots) {
                histo->plot(name, scanOpts.outputDir);
            }
            // always dump the data
            histo->toFile(name, scanOpts.outputDir);
        } // while
    } // i
    logger->info("Finishing run: {}", runCounter);
    // Register test info into database
    if (scanOpts.dbUse) {
        database->cleanUp("scan", scanOpts.outputDir, true, false, scanOpts.dbTag );
    }

    if (scanOpts.makeGraph) {
        diagram.getStats();
        diagram.toFile(scanOpts.outputDir + "diagram.json");
        if (scanOpts.doPlots) {
            diagram.toPlot(scanOpts.outputDir + "diagram.png");
        }
    }
}

std::string ScanConsoleImpl::getResults() {
    std::string str;
    json result;
    getResults(result);
    result.dump(str);
    return str;
}

void ScanConsoleImpl::getResults(json &result) {
    json frontends;
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        if (fe->isActive()) {
            auto feCfg = dynamic_cast<FrontEndCfg *>(fe);
            std::string name = feCfg->getName();
            json jTmp;
            feCfg->writeConfig(jTmp);
            frontends[name]["configs"] = jTmp;
            auto &output = *(fe->clipResult.back());
            json histos;
            while (!output.empty()) {
                json h;
                auto histo = output.popData();
                histo->toJson(h);
                histos[h["Name"]]=h;
            }
            frontends[name]["histos"] = histos;
        }
    }
    result["frontends"] = frontends;
    result["scanLog"] = scanLog;
}

void ScanConsoleImpl::run() {
    ScanHelper::banner(logger,"Run Scan");

    scan_start = std::chrono::steady_clock::now();
    scanBase->run();
    scanBase->postScan();
    logger->info("Scan done!");

    // Join from upstream to downstream.
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        if (fe->isActive()) {
          fe->clipRawData.finish();
        }
    }

    scan_done = std::chrono::steady_clock::now();
    logger->info("Waiting for processors to finish ...");
    // Join Fei4DataProcessor
    for( auto& proc : procs ) {
      proc.second->join();
    }
    processor_done = std::chrono::steady_clock::now();
    logger->info("Processor done, waiting for histogrammer ...");

    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        if (fe->isActive()) {
          fe->clipData.finish();
        }
    }

    // Join histogrammers
    for( auto& histogrammer : histogrammers ) {
      histogrammer.second->join();
    }

    logger->info("Processor done, waiting for analysis ...");

    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        FrontEnd *fe = bookie->getEntry(id).fe;
        if (fe->isActive()) {
          fe->clipHisto.finish();
        }
    }

    // Join analyses
    for( auto& ana : analyses ) {
      FrontEnd *fe = bookie->getEntry(ana.first).fe;
      for (unsigned i=0; i<ana.second.size(); i++) {
        ana.second[i]->join();
        // Also declare done for its output ClipBoard
        fe->clipResult.at(i)->finish();
      }
    }

    all_done = std::chrono::steady_clock::now();
    logger->info("All done!");

    // Joining is done.

    hwCtrl->disableCmd();
    hwCtrl->disableRx();
    ScanHelper::banner(logger,"Timing");
    logger->info("-> Configuration: {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count());
    logger->info("-> Scan:          {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(scan_done-scan_start).count());
    logger->info("-> Processing:    {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(processor_done-scan_done).count());
    logger->info("-> Analysis:      {} ms", std::chrono::duration_cast<std::chrono::milliseconds>(all_done-processor_done).count());

    scanLog["stopwatch"]["config"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count();
    scanLog["stopwatch"]["scan"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(scan_done-scan_start).count();
    scanLog["stopwatch"]["processing"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(processor_done-scan_done).count();
    scanLog["stopwatch"]["analysis"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(all_done-processor_done).count();
    hwCtrl.reset();
    scanLog["finishTime"] = (int)std::time(nullptr);

}

void ScanConsoleImpl::dump() {
    scanConsoleConfig.dump();
    scanLog.dump();
}

void ScanConsoleImpl::setupLogger(const char *config) {
    json loggerConfig;
    loggerConfig["pattern"] = "[%T:%e]%^[%=8l][%=15n][%t]:%$ %v";
    loggerConfig["log_config"][0]["name"] = "all";
    loggerConfig["log_config"][0]["level"] = "info";
    loggerConfig["outputDir"] = "";
    if (config) {
        try {
            json::parse(config, loggerConfig);
        } catch (...) {}
    }
    logging::setupLoggers(loggerConfig);
}
