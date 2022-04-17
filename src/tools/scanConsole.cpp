// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Command line scan tool
// # Comment: To be used instead of gui
// ################################

#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>
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

#include "yarr.h"

auto logger = logging::make_log("scanConsole");

int main(int argc, char *argv[]) {
    ScanOpts scanOpts;
    spdlog::set_pattern(scanOpts.defaultLogPattern);
    ScanHelper::banner(logger,"Welcome to the YARR Scan Console!");

    spdlog::info("-> Parsing command line parameters ...");

    int res=ScanHelper::parseOptions(argc,argv,scanOpts);
    if(res<=0) exit(res);

    unsigned runCounter=0;
    std::string strippedScan;
    std::string dataDir;
    json scanLog;
    std::unique_ptr<HwController> hwCtrl;
    std::unique_ptr<Bookkeeper> bookie;
    std::map<FrontEnd*, std::array<std::string, 2>> feCfgMap;
    std::unique_ptr<ScanBase> scanBase;
    std::map<FrontEnd*, std::unique_ptr<DataProcessor> > histogrammers;
    std::map<FrontEnd*, std::vector<std::unique_ptr<DataProcessor>> > analyses;
    std::shared_ptr<DataProcessor> proc;
    std::string chipType;
    std::string timestampStr;
    std::time_t now;

    // Get new run number
    runCounter = ScanHelper::newRunCounter();

    // parsing all json files
    json loggerConfig;
    json chipConfig;
    json dbCfg;
    json ctrlCfg;
    json userCfg;
    json siteCfg;
    json scanCfg;

    json scanConsoleConfig;
    std::unique_ptr<DBHandler> database;

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


    int result = ScanHelper::loadConfigFile(scanOpts, true, scanConsoleConfig);
    if(result<0) {
        spdlog::error("Failed to read configs");
        exit(-1);
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

    // end of configuration section

    // create outdir directory
    dataDir=scanOpts.outputDir;
    strippedScan=ScanHelper::createOutputDir(scanOpts.scanType,runCounter,scanOpts.outputDir);

    if (scanOpts.cConfigPaths.empty()) {
        logger->error("Error: no config files given, please specify config file name under -c option, even if file does not exist!");
        return -1;
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

    // Make symlink
    ScanHelper::createSymlink(dataDir,strippedScan,runCounter);

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
        for (FrontEnd* fe : bookie->feList) {
            fe->enableAll();
        }
    }
    for ( FrontEnd* fe : bookie->feList ) {
       auto feCfg = dynamic_cast<FrontEndCfg*>(fe);
       ScanHelper::writeFeConfig(feCfg, scanOpts.outputDir + feCfgMap.at(fe)[1] + ".before");
    }
    bookie->initGlobalFe(StdDict::getFrontEnd(chipType).release());
    bookie->getGlobalFe()->makeGlobal();
    bookie->getGlobalFe()->init(&*hwCtrl, 0, 0);

    ScanHelper::banner(logger,"Configure FEs");

    std::chrono::steady_clock::time_point cfg_start = std::chrono::steady_clock::now();

    // Before configuring each FE, broadcast reset to all tx channels
    // Enable all tx channels
    hwCtrl->setCmdEnable(bookie->getTxMaskUnique());
    // Use global FE
    bookie->getGlobalFe()->resetAll();

    for ( FrontEnd* fe : bookie->feList ) {
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
    std::chrono::steady_clock::time_point cfg_end = std::chrono::steady_clock::now();
    logger->info("Sent configuration to all FEs in {} ms!",
                 std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count());

    // Wait for rx to sync with FE stream
    // TODO Check RX sync
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    hwCtrl->flushBuffer();
    for ( FrontEnd* fe : bookie->feList ) {
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
        logger->info("... success!");
    }

    // at this point, if we're not running a scan we should just exit
    if(!scanOpts.scan_config_provided) {
        return 0;
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

    //hwCtrl->runMode();

    ScanHelper::banner(logger,"Setup Scan");

    // Make backup of scan config

    // Create backup of current config
    if (scanOpts.scanType.find("json") != std::string::npos) {
        // TODO fix folder
        std::ifstream cfgFile(scanOpts.scanType);
        std::ofstream backupCfgFile(scanOpts.outputDir + strippedScan + ".json");
        backupCfgFile << cfgFile.rdbuf();
        backupCfgFile.close();
        cfgFile.close();
    }

    // For sending feedback data
    FeedbackClipboardMap fbData;

    // TODO Make this nice
    try {
        scanBase = ScanHelper::buildScan(scanCfg, *bookie, &fbData);
    } catch (const char *msg) {
        logger->warn("No scan to run, exiting with msg: {}", msg);
        return 0;
    }
    // TODO not to use the raw pointer!
    try {
        ScanHelper::buildHistogrammers(histogrammers, scanCfg, bookie->feList, scanBase.get(), scanOpts.outputDir);
    } catch (const char *msg) {
        logger->error("{}", msg);
        return -1;
    }

    try {
        ScanHelper::buildAnalyses(analyses, scanCfg, *bookie, scanBase.get(),
                                  &fbData, scanOpts.mask_opt);
    } catch (const char *msg) {
        logger->error("{}", msg);
        return -1;
    }

    logger->info("Running pre scan!");
    scanBase->init();
    scanBase->preScan();

    // Run from downstream to upstream
    logger->info("Starting histogrammer and analysis threads:");
    for ( FrontEnd* fe : bookie->feList ) {
        if (!fe->isActive()) continue;
      for (auto& ana : analyses[fe]) {
        ana->init();
        ana->run();
      }
      histogrammers[fe]->init();
      histogrammers[fe]->run();
      logger->info(" .. started threads of Fe {}", dynamic_cast<FrontEndCfg*>(fe)->getRxChannel());
    }

    proc = StdDict::getDataProcessor(chipType);
    //Fei4DataProcessor proc(bookie.globalFe<Fei4>()->getValue(&Fei4::HitDiscCnfg));
    proc->connect( &bookie->rawData, &bookie->eventMap );
    if(scanOpts.nThreads>0) proc->setThreads(scanOpts.nThreads); // override number of used threads
    proc->init();
    proc->run();

    // Now the all downstream processors are ready --> Run scan

    ScanHelper::banner(logger,"Scan");

    logger->info("Starting scan!");
    std::chrono::steady_clock::time_point scan_start = std::chrono::steady_clock::now();
    scanBase->run();
    scanBase->postScan();
    logger->info("Scan done!");

    // Join from upstream to downstream.

    bookie->rawData.finish();

    std::chrono::steady_clock::time_point scan_done = std::chrono::steady_clock::now();
    logger->info("Waiting for processors to finish ...");
    // Join Fei4DataProcessor
    proc->join();
    std::chrono::steady_clock::time_point processor_done = std::chrono::steady_clock::now();
    logger->info("Processor done, waiting for histogrammer ...");

    for (auto fe : bookie->feList) {
        if (fe->isActive()) {
          fe->clipData->finish();
        }
    }

    // Join histogrammers
    for( auto& histogrammer : histogrammers ) {
      histogrammer.second->join();
    }

    logger->info("Processor done, waiting for analysis ...");

    for (auto fe : bookie->feList) {
        if (fe->isActive()) {
          fe->clipHisto->finish();
        }
    }

    // Join analyses
    for( auto& ana : analyses ) {
      FrontEnd *fe = ana.first;
      for (unsigned i=0; i<ana.second.size(); i++) {
        ana.second[i]->join();
        // Also declare done for its output ClipBoard
        fe->clipResult->at(i)->finish();
      }
    }

    std::chrono::steady_clock::time_point all_done = std::chrono::steady_clock::now();
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

    ScanHelper::banner(logger,"Cleanup");

    // Call constructor (eg shutdown Emu threads)
    hwCtrl.reset();
    scanLog["finishTime"] = (int)std::time(nullptr);
    ScanHelper::writeScanLog(scanLog, scanOpts.outputDir + "scanLog.json");

    // Cleanup
    //delete scanBase;
    for (auto fe : bookie->feList) {
        if(!fe->isActive()) continue;
        auto feCfg = dynamic_cast<FrontEndCfg*>(fe);

        // Save config
        if (!feCfg->isLocked()) {
            const std::string &filename=feCfgMap.at(fe)[0];
            logger->info("Saving config of FE {} to {}",
                         feCfg->getName(), filename);
            ScanHelper::writeFeConfig(feCfg, filename);
        } else {
            logger->warn("Not saving config for FE {} as it is protected!", feCfg->getName());
        }

        // Save extra config in data folder
        ScanHelper::writeFeConfig(feCfg, scanOpts.outputDir + feCfgMap.at(fe)[1] + ".after");

        // Plot
        // store output results (if any)
        if(analyses.empty()) continue;
        logger->info("-> Storing output results of FE {}", feCfg->getRxChannel());
        auto &output = *(fe->clipResult->back());
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
    if(scanOpts.doPlots) {
        bool ok = ScanHelper::lsdir(dataDir+ "last_scan/");
        if(!ok)
            logger->info("Find plots in: {}last_scan", dataDir);
    }

    // Register test info into database
    if (scanOpts.dbUse) {
        database->cleanUp("scan", scanOpts.outputDir, false, false);
    }

    return 0;
}

