//////////////////////////////////////////////////
// Utility to run SEE testing in a CLI interface,
// with traditional scan console parameters for
// controller and connectivity files.
//
// author: Luc Le Pottier
// e-mail: luclepot@lbl.gov
// May 2024
//////////////////////////////////////////////////

// std/stl
#include <iostream>
#include <string>
#include <sstream>
#include <getopt.h>

#include <filesystem>
namespace fs = std::filesystem;
#include <memory> // unique_ptr

// YARR
#include "HwController.h"
#include "FrontEnd.h"
#include "AllChips.h"
#include "ScanHelper.h" // openJson
#include "Utils.h"
#include "logging.h"
#include "LoggingConfig.h"
#include "ScanOpts.h"
#include "ScanFactory.h"

namespace {
    auto logger = logging::make_log("seeTest");
}

class seeAnalysis  : public HistogramAlgorithm {
    public:
        seeAnalysis() {

        }

        void processEvent(FrontEndData *data) override {
            for(auto eventIt = data->begin(); eventIt < data->end(); eventIt++) {

            }
        }
    private:

};

int loadConfigs(int argc, char **argv, ScanOpts &scanOpts, json& scanConsoleConfig) {
    int res = ScanHelper::parseOptions(argc, argv, scanOpts);
    std::string strippedScan;
    json loggerConfig;
    std::cout << "Setting up Logger" << std::endl;
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

    logging::setupLoggers(loggerConfig);
    ScanHelper::banner(logger,"Welcome to YARR - SEE Testing");

    // Possibly replace this with a hard-coded scan config?
    // Less natural for YARR, however
    if(!scanOpts.scan_config_provided) {
        logger->critical("SEE testing scan config is required!");
    }

    if (res==1) {
        res = ScanHelper::loadConfigFile(scanOpts, true, scanConsoleConfig);
        if (res < 0) {
            logger->error("Failed to read configs.");
            return -1;
        }
    }
    else if (res == 0) {
        return 1;
    }
    else {
        return res;
    }



    unsigned runCounter = ScanHelper::newRunCounter();

    // Create output directory
    std::string dataDir = scanOpts.outputDir;
    if(scanOpts.doOutput) {
        strippedScan = ScanHelper::createOutputDir("see_testing_scan", runCounter, scanOpts.outputDir);
        ScanHelper::createSymlink(dataDir, strippedScan, runCounter);
    }
    scanConsoleConfig["strippedScan"] = strippedScan;
    scanConsoleConfig["runCounter"] = runCounter;
    
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

int loadHardware(ScanOpts &scanOpts, json &scanConsoleConfig, std::map<unsigned, std::array<std::string, 2>> & feCfgMap, std::unique_ptr<HwController> &hwCtrl, std::unique_ptr<Bookkeeper> &bookie) {
    ScanHelper::banner(logger,"Initalizing Hardware");
    json chipConfig = scanConsoleConfig["chipConfig"];

    std::string chipType{};

    for(json const& config : chipConfig){
        try {
            chipType = ScanHelper::buildChips(config, *bookie, &*hwCtrl, feCfgMap);
        } catch (std::runtime_error &e) {
            logger->critical("#ERROR# loading chip config: {}", e.what());
            return -1;
        }
    }
    // pass on ChipType
    
    scanConsoleConfig["chipType"] = chipType;
    // Reset masks
    if (scanOpts.mask_opt == 1) {
        for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
            auto feCfg = bookie->getFeCfg(id);
            feCfg->enableAll();
        }
    }
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        auto feCfg = bookie->getFeCfg(id);
        if(scanOpts.doOutput)
            ScanHelper::writeFeConfig(feCfg, scanOpts.outputDir + feCfgMap.at(id)[1] + ".before");
    }

    bookie->initGlobalFe(chipType);
    bookie->getGlobalFe()->init(&*hwCtrl, FrontEndConnectivity(0,0));
    
    return 0;
}

int configure(ScanOpts &scanOpts, json &scanConsoleConfig, std::map<unsigned, std::array<std::string, 2>> & feCfgMap, std::unique_ptr<HwController> &hwCtrl, std::unique_ptr<Bookkeeper> &bookie) {

    std::chrono::steady_clock::time_point cfg_start, cfg_end;
    // Reset masks
    if (scanOpts.mask_opt == 1) {
        for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
            auto feCfg = bookie->getFeCfg(id);
            feCfg->enableAll();
        }
    }
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        auto feCfg = bookie->getFeCfg(id);
        if(scanOpts.doOutput)
            ScanHelper::writeFeConfig(feCfg, scanOpts.outputDir + feCfgMap.at(id)[1] + ".before");
    }
    bookie->initGlobalFe(scanConsoleConfig["chipType"]);
    bookie->getGlobalFe()->init(&*hwCtrl, FrontEndConnectivity(0,0));

    ScanHelper::banner(logger, "Configure FEs");

    cfg_start = std::chrono::steady_clock::now();

    // Before configuring each FE, broadcast reset to all tx channels
    // Enable all tx channels
    hwCtrl->setCmdEnable(bookie->getTxMaskUnique());

    // send global/broadcast reset command to all frontends
    if(scanOpts.doResetBeforeScan) {
        bookie->getGlobalFe()->resetAllHard();
    }

    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        auto feCfg = bookie->getFeCfg(id);
        logger->info("Configuring {}", feCfg->getName());
        // Select correct channel
        hwCtrl->setCmdEnable(feCfg->getTxChannel());
        // Configure
        bookie->getFe(id)->configure();
        // Wait for fifo to be empty
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        while(!hwCtrl->isCmdEmpty());
    }
    cfg_end = std::chrono::steady_clock::now();
    logger->info("Sent configuration to all FEs in {} ms!",
                 std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count());

    hwCtrl->setCmdEnable(bookie->getTxMaskUnique());
    // send global/broadcast soft reset post config
    bookie->getGlobalFe()->resetAllSoft();
    // Wait for rx to sync with FE stream
    // TODO Check RX sync
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    hwCtrl->flushBuffer();
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        auto fe = bookie->getFe(id);
        auto feCfg = bookie->getFeCfg(id);
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

int setupScan(ScanOpts &scanOpts, json &scanCfg, json &scanConsoleConfig, std::unique_ptr<ScanFactory> &s, FeedbackClipboardMap &fbData) {
    if (scanOpts.doOutput &&
        scanOpts.scanType.find("json") != std::string::npos) {
        // TODO fix folder
        std::ifstream cfgFile(scanOpts.scanType);
        std::ofstream backupCfgFile(scanOpts.outputDir + std::string(scanConsoleConfig["strippedScan"]) + ".json");
        backupCfgFile << cfgFile.rdbuf();
        backupCfgFile.close();
        cfgFile.close();
    }

    s->loadConfig(scanCfg);
    return 0;
}

int main(int argc, char *argv[]) {

    // initalize locals
    int rval;
    std::string strippedScan;
    ScanOpts scanOpts;
    FeedbackClipboardMap fbData;
    json scanConsoleConfig;
    std::map<unsigned, std::array<std::string, 2>> feCfgMap;
    std::map<unsigned, std::unique_ptr<FeDataProcessor> > procs{};
    std::map<unsigned, std::unique_ptr<HistoDataProcessor> > histogrammers{};
    std::map<unsigned, std::vector<std::unique_ptr<AnalysisDataProcessor>> > analyses{};

    // load configs
    rval = loadConfigs(argc, argv, scanOpts, scanConsoleConfig); if(rval != 0) return rval;
    json ctrlCfg = scanConsoleConfig["ctrlConfig"];
    json chipConfig = scanConsoleConfig["chipConfig"];
    json scanCfg = scanConsoleConfig["scanCfg"];

    
    // load hardware
    std::unique_ptr<HwController> hwCtrl = ScanHelper::loadController(ctrlCfg);
    std::unique_ptr<Bookkeeper> bookie = std::make_unique<Bookkeeper>(&*hwCtrl, &*hwCtrl);
    rval = loadHardware(scanOpts, scanConsoleConfig, feCfgMap, hwCtrl, bookie); if(rval != 0) return rval;

    // configure
    rval = configure(scanOpts, scanConsoleConfig, feCfgMap, hwCtrl, bookie);

    std::unique_ptr<ScanFactory> s ( new ScanFactory(&*bookie, &fbData) );
    setupScan(scanOpts, scanCfg, scanConsoleConfig, s, fbData);


    // Build things
    try {
        ScanHelper::buildRawDataProcs(procs, *bookie, scanConsoleConfig["chipType"]);
        ScanHelper::buildHistogrammers(histogrammers, scanCfg, *bookie, scanOpts.outputDir);
        ScanHelper::buildAnalyses(analyses, scanCfg, *bookie, s.get(),
                                  &fbData, scanOpts.mask_opt, scanOpts.outputDir,
                                  scanOpts.target_tot, scanOpts.target_charge);
    } catch (const char *msg) {
        logger->error("{}", msg);
        return -1;
    }

    // Prescan
    logger->info("Running pre scan!");
    s->init();
    s->preScan();

    // Run from downstream to upstream
    logger->info("Starting histogrammer and analysis threads:");
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        if (!(bookie->getFe(id)->isActive())) continue;
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


    /// THIS IS WHERE THE SEE STUFF GOES

    logger->info("Scan done!");

    // Join from upstream to downstream.
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        auto fe = bookie->getFe(id);
        if (fe->isActive()) {
          fe->clipRawData.finish();
        }
    }

    // scan_done = std::chrono::steady_clock::now();
    logger->info("Waiting for processors to finish ...");
    // Join Fei4DataProcessor
    for( auto& proc : procs ) {
      proc.second->join();
    }
    // processor_done = std::chrono::steady_clock::now();
    logger->info("Processor done, waiting for histogrammer ...");

    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        auto fe = bookie->getFe(id);
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
        auto fe = bookie->getFe(id);
        if (fe->isActive()) {
          fe->clipHisto.finish();
        }
    }

    // Join analyses
    for( auto& ana : analyses ) {
      auto fe = bookie->getFe(ana.first);
      for (unsigned i=0; i<ana.second.size(); i++) {
        ana.second[i]->join();
        // Also declare done for its output ClipBoard
        fe->clipResult.at(i)->finish();
      }
    }

    // all_done = std::chrono::steady_clock::now();
    logger->info("All done!");

    hwCtrl->disableCmd();
    hwCtrl->disableRx();

    ScanHelper::banner(logger,"Cleanup");

    // Cleanup
    for (unsigned id=0; id<bookie->getNumOfEntries(); id++) {
        auto fe = bookie->getFe(id);
        if(!fe->isActive()) continue;
        auto feCfg = bookie->getFeCfg(id);

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
    logger->info("Finishing run: {}", (int)scanConsoleConfig["runCounter"]);

    hwCtrl.reset();
    bookie.reset();

    // run scan
    // cleanup
    // plot
    // done
    logger->info("Exiting SEE testing");

    return 0;
}
