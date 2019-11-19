// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Command line scan tool
// # Comment: To be used instead of gui
// ################################

#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <iomanip>
#include <cctype> //w'space detection
#include <ctime>
#include <map>
#include <sstream>

#include "logging.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "ScanHelper.h"

#include "HwController.h"

#include "AllHwControllers.h"
#include "AllChips.h"
#include "AllProcessors.h"

#include "Bookkeeper.h"
#include "Fei4.h"
#include "ScanBase.h"
#include "ScanFactory.h"
#include "Fei4DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "Fei4Analysis.h"

#include "DBHandler.h"
#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)

//  #include <errno.h>
//  #include <sys/stat.h>
#include <cstdlib> //I am not proud of this ):

#endif

#include "storage.hpp"

auto logger = logging::make_log("scan_console");

std::string toString(int value,int digitsCount)
{
    std::ostringstream os;
    os<<std::setfill('0')<<std::setw(digitsCount)<<value;
    return os.str();
}

void printHelp();
void listScans();
void listKnown();

std::unique_ptr<ScanBase> buildScan( const std::string& scanType, Bookkeeper& bookie );

// In order to build Histogrammer, bookie is not needed --> good sign!
// Do not want to use the raw pointer ScanBase*
void buildHistogrammers( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& histogrammers, const std::string& scanType, std::vector<FrontEnd*>& feList, ScanBase* s, std::string outputDir);

// In order to build Analysis, bookie is needed --> deep dependency!
// Do not want to use the raw pointer ScanBase*
void buildAnalyses( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& analyses, const std::string& scanType, Bookkeeper& bookie, ScanBase* s, int mask_opt);

void setupLoggers(const json &j);

int main(int argc, char *argv[]) {
    std::cout << "\033[1;31m#####################################\033[0m" << std::endl;
    std::cout << "\033[1;31m# Welcome to the YARR Scan Console! #\033[0m" << std::endl;
    std::cout << "\033[1;31m#####################################\033[0m" << std::endl;

    std::cout << "-> Parsing command line parameters ..." << std::endl;
    
    std::string home = getenv("HOME");
    std::string hostname = "default_host";
    // HOSTNAME does not exist like this on mac, need to work around it
    if (getenv("HOSTNAME")) {
        hostname = getenv("HOSTNAME");
    } else {
        std::cout << "HOSTNAME environmental variable not found ..." << std::endl;
    }

    // Init parameters
    std::string scanType = "";
    std::vector<std::string> cConfigPaths;
    std::string outputDir = "./data/";
    std::string ctrlCfgPath = "";
    bool doPlots = false;
    int target_charge = -1;
    int target_tot = -1;
    int mask_opt = -1;

    bool dbUse = false;
    std::string dbDirPath = home+"/.yarr/localdb";
    std::string dbCfgPath = dbDirPath+"/"+hostname+"_database.json";
    std::string dbSiteCfgPath = "";
    std::string dbUserCfgPath = "";
    std::string logCfgPath = "";
    
    unsigned runCounter = 0;

    // Load run counter
    if (system("mkdir -p ~/.yarr") < 0) {
        std::cerr << "#ERROR# Loading run counter ~/.yarr!" << std::endl;
    }
    
    std::fstream iF((home + "/.yarr/runCounter").c_str(), std::ios::in);
    if (iF) {
        iF >> runCounter;
        runCounter += 1;
    } else {
        if (system("echo \"1\n\" > ~/.yarr/runCounter") < 0) {
            std::cerr << "#ERROR# trying to run echo!" << std::endl;
        }
        runCounter = 1;
    }
    iF.close();

    std::fstream oF((home + "/.yarr/runCounter").c_str(), std::ios::out);
    oF << runCounter << std::endl;
    oF.close();

    int nThreads = 4;
    int c;
    while ((c = getopt(argc, argv, "hn:ks:n:m:g:r:c:t:po:Wd:u:i:l:")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 'n':
                nThreads=atoi(optarg);
                break;
            case 'k':
                listKnown();
                return 0;
            case 's':
                scanType = std::string(optarg);
                break;
            case 'm':
                mask_opt = atoi(optarg);
                break;
            case 'c':
//                configPath = std::string(optarg);
                optind -= 1; //this is a bit hacky, but getopt doesn't support multiple
                             //values for one option, so it can't be helped
                for(; optind < argc && *argv[optind] != '-'; optind += 1){
                    cConfigPaths.push_back(std::string(argv[optind]));
                }
                break;
            case 'r':
                ctrlCfgPath = std::string(optarg);
                break;
            case 'p':
                doPlots = true;
                break;
            case 'o':
                outputDir = std::string(optarg);
                if (outputDir.back() != '/')
                    outputDir = outputDir + "/";
                break;
            case 't':
                optind -= 1; //this is a bit hacky, but getopt doesn't support multiple
                             //values for one option, so it can't be helped
                for(; optind < argc && *argv[optind] != '-'; optind += 1){
                    switch (count) {
                        case 0:
                            target_charge = atoi(argv[optind]);
                            break;
                        case 1:
                            target_tot = atoi(argv[optind]);
                            break;
                        default:
                            std::cerr << "-> Can only receive max. 2 parameters with -t!!" << std::endl;
                            break;
                    }
                    count++;
                }
                break;
            case 'W': // Write to DB
                dbUse = true;
                break;
            case 'd': // Database config file
                dbCfgPath = std::string(optarg);
                break;
            case 'l': // Logger config file
                logCfgPath = std::string(optarg);
                break;
            case 'i': // Database config file
                dbSiteCfgPath = std::string(optarg);
                break;
            case 'u': // Database config file
                dbUserCfgPath = std::string(optarg);
                break;
            case '?':
                if(optopt == 's' || optopt == 'n'){
                    std::cerr << "-> Option " << (char)optopt
                              << " requires a parameter! (Proceeding with default)"
                              << std::endl;
                }else if(optopt == 'g' || optopt == 'c'){
                    std::cerr << "-> Option " << (char)optopt
                              << " requires a parameter! Aborting... " << std::endl;
                    return -1;
                } else {
                    std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
                }
                break;
            default:
                std::cerr << "-> Error while parsing command line parameters!" << std::endl;
                return -1;
        }
    }

    if (cConfigPaths.size() == 0) {
        std::cerr << "Error: no config files given, please specify config file name under -c option, even if file does not exist!" << std::endl;
        return -1;
    }

    std::size_t pathPos = scanType.find_last_of('/');
    std::size_t suffixPos = scanType.find_last_of('.');
    std::string strippedScan;
    if (pathPos != std::string::npos && suffixPos != std::string::npos) {
        strippedScan = scanType.substr(pathPos+1, suffixPos-pathPos-1);
    } else {
        strippedScan = scanType;
    }

    std::string dataDir = outputDir;
    outputDir += (toString(runCounter, 6) + "_" + strippedScan + "/");
    
    std::cout << " Scan Type/Config: " << scanType << std::endl;
    
    std::cout << " Connectivity: " << std::endl;
    for(std::string const& sTmp : cConfigPaths){
        std::cout << "    " << sTmp << std::endl;
    }
    std::cout << " Target ToT: " << target_tot << std::endl;
    std::cout << " Target Charge: " << target_charge << std::endl;
    std::cout << " Output Plots: " << doPlots << std::endl;
    std::cout << " Output Directory: " << outputDir << std::endl;

    // Create folder
    //for some reason, 'make' issues that mkdir is an undefined reference
    //a test program on another machine has worked fine
    //a test program on this machine has also worked fine
    //    int mDExSt = mkdir(outputDir.c_str(), 0777); //mkdir exit status
    //    mode_t myMode = 0777;
    //    int mDExSt = mkdir(outputDir.c_str(), myMode); //mkdir exit status
    std::string cmdStr = "mkdir -p "; //I am not proud of this ):
    cmdStr += outputDir;
    int sysExSt = system(cmdStr.c_str());
    if(sysExSt != 0){
        std::cerr << "Error creating output directory - plots might not be saved!" << std::endl;
    }
    //read errno variable and catch some errors, if necessary
    //errno=1 is permission denied, errno = 17 is dir already exists, ...
    //see /usr/include/asm-generic/errno-base.h and [...]/errno.h for all codes
    
    // Make symlink
    cmdStr = "rm -f " + dataDir + "last_scan && ln -s " + toString(runCounter, 6) + "_" + strippedScan + " " + dataDir + "last_scan";
    sysExSt = system(cmdStr.c_str());
    if(sysExSt != 0){
        std::cerr << "Error creating symlink to output directory!" << std::endl;
    }

    if(!logCfgPath.empty()) {
      auto j = ScanHelper::openJsonFile(logCfgPath);
      setupLoggers(j);
    }

    // Timestamp
    std::time_t now = std::time(NULL);
    struct tm *lt = std::localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%F_%H:%M:%S", lt);
    std::cout << std::endl;
    std::cout << "Timestamp: " << timestamp << std::endl;
    std::cout << "Run Number: " << runCounter;

    std::string commandLineStr= "";
    for (int i=1;i<argc;i++) commandLineStr.append(std::string(argv[i]).append(" "));
    
    json scanLog;
    // Add to scan log
    scanLog["exec"] = commandLineStr;
    scanLog["timestamp"] = timestamp;
    scanLog["startTime"] = (int)now;
    scanLog["runNumber"] = runCounter;
    scanLog["targetCharge"] = target_charge;
    scanLog["targetTot"] = target_tot;
    scanLog["testType"] = strippedScan;

    // Initial setting local DBHandler
    DBHandler *database = new DBHandler();
    if (dbUse) {
        std::cout << std::endl;
        std::cout << "\033[1;31m################\033[0m" << std::endl;
        std::cout << "\033[1;31m# Set Database #\033[0m" << std::endl;
        std::cout << "\033[1;31m################\033[0m" << std::endl;
        std::cout << "-> Setting user's information" << std::endl;

        json dbCfg;
        try {
            dbCfg = ScanHelper::openJsonFile(dbCfgPath);
        } catch (std::runtime_error &e) {
            std::cerr << "#DB ERROR# opening or loading database config: " << e.what() << std::endl;
            return -1;
        }
        scanLog["dbCfg"] = dbCfg;

        database->initialize(dbCfgPath, argv[0]); 

        // set/check user config if specified
        json userCfg = database->setUser(dbUserCfgPath);
        scanLog["userCfg"] = userCfg;
        // set/check site config if specified
        json siteCfg = database->setSite(dbSiteCfgPath);
        scanLog["siteCfg"] = siteCfg;
    }
    std::cout << std::endl;
    std::cout << "\033[1;31m#################\033[0m" << std::endl;
    std::cout << "\033[1;31m# Init Hardware #\033[0m" << std::endl;
    std::cout << "\033[1;31m#################\033[0m" << std::endl;

    logger->info("-> Opening controller config: {}", ctrlCfgPath);

    std::unique_ptr<HwController> hwCtrl = nullptr;
    json ctrlCfg;
    try {
        ctrlCfg = ScanHelper::openJsonFile(ctrlCfgPath);
        hwCtrl = ScanHelper::loadController(ctrlCfg);
    } catch (std::runtime_error &e) {
        std::cerr << "#ERROR# opening or loading controller config: " << e.what() << std::endl;
        return -1;
    }
    // Add to scan log
    scanLog["ctrlCfg"] = ctrlCfg;
    
    hwCtrl->setupMode();

    // Disable trigger in-case
    hwCtrl->setTrigEnable(0);
 
    Bookkeeper bookie(&*hwCtrl, &*hwCtrl);

    std::map<FrontEnd*, std::string> feCfgMap;

    bookie.setTargetTot(target_tot);
    bookie.setTargetCharge(target_charge);

    std::cout << "\033[1;31m#######################\033[0m" << std::endl
              << "\033[1;31m##  Loading Configs  ##\033[0m" << std::endl
              << "\033[1;31m#######################\033[0m" << std::endl;

    int success = 0;
    std::string chipType;

    // Loop over setup files
    for(std::string const& sTmp : cConfigPaths){
        logger->info("Opening global config: {}", sTmp);
        json config;
        try {
            config = ScanHelper::openJsonFile(sTmp);
            chipType = ScanHelper::loadChips(config, bookie, &*hwCtrl, feCfgMap, outputDir);
        } catch (std::runtime_error &e) {
            std::cerr << "#ERROR# opening connectivity or chip configs: " << e.what() << std::endl;
            return -1;
        }
        scanLog["connectivity"].push_back(config);
    }

    if (dbUse) {
        logger->info("Setting Connectivity Configs");
        // set/check connectivity config files
        database->setConnCfg(cConfigPaths);
    }
    
    // Reset masks
    if (mask_opt == 1) {
        for (FrontEnd* fe : bookie.feList) {
            // TODO make mask generic?
            if (chipType == "FEI4B") {
                std::cout << "Resetting enable/hitbus pixel mask to all enabled!" << std::endl;
                for (unsigned int dc = 0; dc < dynamic_cast<Fei4*>(fe)->n_DC; dc++) {
                    dynamic_cast<Fei4*>(fe)->En(dc).setAll(1);
                    dynamic_cast<Fei4*>(fe)->Hitbus(dc).setAll(0);
                }
            } else if (chipType == "RD53A") {
                std::cout << "Resetting enable/hitbus pixel mask to all enabled!" << std::endl;
                for (unsigned int col = 0; col < dynamic_cast<Rd53a*>(fe)->n_Col; col++) {
                    for (unsigned row = 0; row < dynamic_cast<Rd53a*>(fe)->n_Row; row ++) {
                        dynamic_cast<Rd53a*>(fe)->setEn(col, row, 1);
                        dynamic_cast<Rd53a*>(fe)->setHitbus(col, row, 1);
                    }
                }
            }
        }
        // TODO add FE65p2
    }
    
    bookie.initGlobalFe(StdDict::getFrontEnd(chipType).release());
    bookie.getGlobalFe()->makeGlobal();
    bookie.getGlobalFe()->init(&*hwCtrl, 0, 0);
    
    std::cout << std::endl;
    std::cout << "\033[1;31m#################\033[0m" << std::endl;
    std::cout << "\033[1;31m# Configure FEs #\033[0m" << std::endl;
    std::cout << "\033[1;31m#################\033[0m" << std::endl;
    
    std::chrono::steady_clock::time_point cfg_start = std::chrono::steady_clock::now();
    for ( FrontEnd* fe : bookie.feList ) {
        logger->info("Configuring {}", dynamic_cast<FrontEndCfg*>(fe)->getName());
        // Select correct channel
        hwCtrl->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        // Configure
        fe->configure();
        // Wait for fifo to be empty
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        while(!hwCtrl->isCmdEmpty());
    }
    std::chrono::steady_clock::time_point cfg_end = std::chrono::steady_clock::now();
    logger->info("All FEs configured in {} ms!",
                 std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count());
    
    // Wait for rx to sync with FE stream
    // TODO Check RX sync
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    hwCtrl->flushBuffer();
    for ( FrontEnd* fe : bookie.feList ) {
        logger->info("Checking com {}", dynamic_cast<FrontEndCfg*>(fe)->getName());
        // Select correct channel
        hwCtrl->setCmdEnable(dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        hwCtrl->setRxEnable(dynamic_cast<FrontEndCfg*>(fe)->getRxChannel());
        // Configure
        if (fe->checkCom() != 1) {
            std::cout << "#ERROR# Can't establish communication, aborting!" << std::endl;
            return -1;
        }
        logger->info("... success!");
    }
 
    // Enable all active channels
    logger->info("Enabling Tx channels");
    hwCtrl->setCmdEnable(bookie.getTxMask());
    for (uint32_t channel : bookie.getTxMask()) {
        logger->info("Enabling Tx channel {}", channel);
    }
    logger->info("Enabling Rx channels");
    hwCtrl->setRxEnable(bookie.getRxMask());
    for (uint32_t channel : bookie.getRxMask()) {
        logger->info("Enabling Rx channel {}", channel);
    }
    
    hwCtrl->runMode();

    std::cout << std::endl;
    std::cout << "\033[1;31m##############\033[0m" << std::endl;
    std::cout << "\033[1;31m# Setup Scan #\033[0m" << std::endl;
    std::cout << "\033[1;31m##############\033[0m" << std::endl;

    // Make backup of scan config
    
    // Create backup of current config
    if (scanType.find("json") != std::string::npos) {
        // TODO fix folder
        std::ifstream cfgFile(scanType);
        std::ofstream backupCfgFile(outputDir + strippedScan + ".json");
        backupCfgFile << cfgFile.rdbuf();
        backupCfgFile.close();
        cfgFile.close();
    }

    // TODO Make this nice 
    std::unique_ptr<ScanBase> s;
    try {
        s = buildScan(scanType, bookie );
    } catch (const char *msg) {
        std::cout << " -> Warning! No scan to run, exiting with msg: " << msg << std::endl;
        return 0;
    }

    // Use the abstract class instead of concrete -- in the future, this will be useful...
    std::map<FrontEnd*, std::unique_ptr<DataProcessor> > histogrammers;
    std::map<FrontEnd*, std::unique_ptr<DataProcessor> > analyses;

    // TODO not to use the raw pointer!
    buildHistogrammers( histogrammers, scanType, bookie.feList, s.get(), outputDir);
    buildAnalyses( analyses, scanType, bookie, s.get(), mask_opt);

    logger->info("Running pre scan!");
    s->init();
    s->preScan();

    // Run from downstream to upstream
    logger->info("Starting histogrammer and analysis threads:");
    for ( FrontEnd* fe : bookie.feList ) {
        if (fe->isActive()) {
          analyses[fe]->init();
          analyses[fe]->run();
          
          histogrammers[fe]->init();
          histogrammers[fe]->run();
          
          logger->info("Analysis thread of Fe {}", dynamic_cast<FrontEndCfg*>(fe)->getRxChannel());
        }
    }

    std::shared_ptr<DataProcessor> proc = StdDict::getDataProcessor(chipType);
    //Fei4DataProcessor proc(bookie.globalFe<Fei4>()->getValue(&Fei4::HitDiscCnfg));
    proc->connect( &bookie.rawData, &bookie.eventMap );
    if(nThreads>0) proc->setThreads(nThreads); // override number of used threads
    proc->init();
    proc->run();

    // Now the all downstream processors are ready --> Run scan

    std::cout << std::endl;
    std::cout << "\033[1;31m########\033[0m" << std::endl;
    std::cout << "\033[1;31m# Scan #\033[0m" << std::endl;
    std::cout << "\033[1;31m########\033[0m" << std::endl;

    logger->info("Starting scan!");
    std::chrono::steady_clock::time_point scan_start = std::chrono::steady_clock::now();
    s->run();
    s->postScan();
    logger->info("Scan done!");

    // Join from upstream to downstream.
    
    bookie.rawData.finish();

    std::chrono::steady_clock::time_point scan_done = std::chrono::steady_clock::now();
    logger->info("Waiting for processors to finish ...");
    // Join Fei4DataProcessor
    proc->join();
    std::chrono::steady_clock::time_point processor_done = std::chrono::steady_clock::now();
    
    logger->info("Processor done, waiting for histogrammer ...");
    
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        if (fe->isActive()) {
          fe->clipData->finish();
        }
    }
    
    // Join histogrammers
    for( auto& histogrammer : histogrammers ) {
      histogrammer.second->join();
    }
    
    logger->info("Processor done, waiting for analysis ...");
    
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        if (fe->isActive()) {
          fe->clipHisto->finish();
        }
    }

    // Join analyses
    for( auto& ana : analyses ) {
      ana.second->join();
    }
      
    std::chrono::steady_clock::time_point all_done = std::chrono::steady_clock::now();
    logger->info("All done!");

    // Joining is done.

    hwCtrl->disableCmd();
    hwCtrl->disableRx();

    std::cout << std::endl;
    std::cout << "\033[1;31m##########\033[0m" << std::endl;
    std::cout << "\033[1;31m# Timing #\033[0m" << std::endl;
    std::cout << "\033[1;31m##########\033[0m" << std::endl;

    std::cout << "-> Configuration: " << std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count() << " ms" << std::endl;
    std::cout << "-> Scan:          " << std::chrono::duration_cast<std::chrono::milliseconds>(scan_done-scan_start).count() << " ms" << std::endl;
    std::cout << "-> Processing:    " << std::chrono::duration_cast<std::chrono::milliseconds>(processor_done-scan_done).count() << " ms" << std::endl;
    std::cout << "-> Analysis:      " << std::chrono::duration_cast<std::chrono::milliseconds>(all_done-processor_done).count() << " ms" << std::endl;
    
    scanLog["stopwatch"]["config"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count();
    scanLog["stopwatch"]["scan"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(scan_done-scan_start).count();
    scanLog["stopwatch"]["processing"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(processor_done-scan_done).count();
    scanLog["stopwatch"]["analysis"] = (uint32_t) std::chrono::duration_cast<std::chrono::milliseconds>(all_done-processor_done).count();

    std::cout << std::endl;
    std::cout << "\033[1;31m###########\033[0m" << std::endl;
    std::cout << "\033[1;31m# Cleanup #\033[0m" << std::endl;
    std::cout << "\033[1;31m###########\033[0m" << std::endl;

    // Call constructor (eg shutdown Emu threads)
    hwCtrl.reset();

    // Save scan log
    now = std::time(NULL);
    scanLog["finishTime"] = (int)now;
    std::ofstream scanLogFile(outputDir + "scanLog.json");
    scanLogFile << std::setw(4) << scanLog;
    scanLogFile.close();

    // Need this folder to plot
    if (system("mkdir -p /tmp/$USER") < 0) {
        std::cerr << "#ERROR# Problem creating /tmp/$USER folder. Plots might work." << std::endl;
    }

    // Cleanup
    //delete s;
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        if (fe->isActive()) {
            
            // Save config
            if (!dynamic_cast<FrontEndCfg*>(fe)->isLocked()) {
                logger->info("Saving config of FE {} to {}",
                             dynamic_cast<FrontEndCfg*>(fe)->getName(), feCfgMap.at(fe));
                json jTmp;
                dynamic_cast<FrontEndCfg*>(fe)->toFileJson(jTmp);
                std::ofstream oFTmp(feCfgMap.at(fe));
                oFTmp << std::setw(4) << jTmp;
                oFTmp.close();
            } else {
                std::cout << "Not saving config for FE " << dynamic_cast<FrontEndCfg*>(fe)->getName() << " as it is protected!" << std::endl;
            }

            // Save extra config in data folder
            std::ofstream backupCfgFile(outputDir + dynamic_cast<FrontEndCfg*>(fe)->getConfigFile() + ".after");
            json backupCfg;
            dynamic_cast<FrontEndCfg*>(fe)->toFileJson(backupCfg);
            backupCfgFile << std::setw(4) << backupCfg;
            backupCfgFile.close(); 

            // Plot
            if (doPlots||dbUse) {
                logger->info("-> Plotting histograms of FE {}",
                             dynamic_cast<FrontEndCfg*>(fe)->getRxChannel());
                std::string outputDirTmp = outputDir;

                auto &output = *fe->clipResult;
                std::string name = dynamic_cast<FrontEndCfg*>(fe)->getName();

                if (output.empty()) {
                    std::cout << " #WARNING# There were no results for chip " << name << ", this usually means that the chip did not send any data at all."
                        << std::endl;
                } else {
                    while(!output.empty()) {
                        std::unique_ptr<HistogramBase> histo = output.popData();
                        histo->plot(name, outputDirTmp);
                        histo->toFile(name, outputDir);
                    }
                }
            }
        }
    }
    std::string lsCmd = "ls -1 " + dataDir + "last_scan/*.p*";
    logger->info("Finishing run: {}", runCounter);
    if(doPlots && (system(lsCmd.c_str()) < 0)) {
        logger->info("Find plots in: {}last_scan", dataDir);
    }

    // Register test info into database
    if (dbUse) {
        database->cleanUp("scan", outputDir);
    }
    delete database;

    return 0;
}

void printHelp() {
    std::string home = getenv("HOME");
    std::string hostname = getenv("HOSTNAME");
    std::string dbDirPath = home+"/.yarr/localdb";

    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -n <threads> : Set number of processing threads." << std::endl;
    std::cout << " -s <scan_type> : Scan config" << std::endl;
    //std::cout << " -n: Provide SPECboard number." << std::endl;
    //std::cout << " -g <cfg_list.txt>: Provide list of chip configurations." << std::endl;
    std::cout << " -c <connectivity.json> [<cfg2.json> ...]: Provide connectivity configuration, can take multiple arguments." << std::endl;
    std::cout << " -r <ctrl.json> Provide controller configuration." << std::endl;
    std::cout << " -t <target_charge> [<tot_target>] : Set target values for threshold/charge (and tot)." << std::endl;
    std::cout << " -p: Enable plotting of results." << std::endl;
    std::cout << " -o <dir> : Output directory. (Default ./data/)" << std::endl;
    std::cout << " -m <int> : 0 = pixel masking disabled, 1 = start with fresh pixel mask, default = pixel masking enabled" << std::endl;
    std::cout << " -k: Report known items (Scans, Hardware etc.)\n";
    std::cout << " -W: Enable using Local DB." << std::endl;
    std::cout << " -d <database.json> : Provide database configuration. (Default " << dbDirPath << "/" << hostname << "_database.json" << std::endl;
    std::cout << " -i <site.json> : Provide site configuration." << std::endl;
    std::cout << " -u <user.json> : Provide user configuration." << std::endl;
}

void listChips() {
    for(std::string &chip_type: StdDict::listFrontEnds()) {
        std::cout << "  " << chip_type << "\n";
    }
}

void listProcessors() {
    for(std::string &proc_type: StdDict::listDataProcessors()) {
        std::cout << "  " << proc_type << "\n";
    }
}

void listScans() {
    for(std::string &scan_name: StdDict::listScans()) {
        std::cout << "  " << scan_name << "\n";
    }
}

void listControllers() {
    for(auto &h: StdDict::listHwControllers()) {
        std::cout << "  " << h << std::endl;
    }
}

void listScanLoopActions() {
    for(auto &la: StdDict::listLoopActions()) {
        std::cout << "  " << la << std::endl;
    }
}

void listLoggers() {
    spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) {
        if(l->name().empty()) {
          std::cout << "  (default)\n";
        } else {
          std::cout << "  " << l->name() << "\n";
        }
    });
}

void listKnown() {
    std::cout << " Known HW controllers:\n";
    listControllers();
    
    std::cout << " Known Chips:\n";
    listChips();

    std::cout << " Known Processors:\n";
    listProcessors();

    std::cout << " Known Scans:\n";
    listScans();

    std::cout << " Known ScanLoop actions:\n";
    listScanLoopActions();

    std::cout << " Known loggers:\n";
    listLoggers();
}

std::unique_ptr<ScanBase> buildScan( const std::string& scanType, Bookkeeper& bookie ) {
    std::unique_ptr<ScanBase> s ( nullptr );

    logger->info("Found Scan config, constructing scan ...");
    s.reset( new ScanFactory(&bookie) );
    json scanCfg;
    try {
        scanCfg = ScanHelper::openJsonFile(scanType);
    } catch (std::runtime_error &e) {
        std::cerr << "#ERROR# opening scan config: " << e.what() << std::endl;
        throw("buildScan failure");
    }
    dynamic_cast<ScanFactory&>(*s).loadConfig(scanCfg);

    return s;
}


void buildHistogrammers( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& histogrammers, const std::string& scanType, std::vector<FrontEnd*>& feList, ScanBase* s, std::string outputDir) {
    logger->info("Found Scan config, loading histogrammer ...");
    json scanCfg;
    try {
        scanCfg = ScanHelper::openJsonFile(scanType);
    } catch (std::runtime_error &e) {
        std::cerr << "#ERROR# opening scan config: " << e.what() << std::endl;
        throw("buildHistogrammer failure");
    }
    json histoCfg = scanCfg["scan"]["histogrammer"];
    json anaCfg = scanCfg["scan"]["analysis"];

    for (FrontEnd *fe : feList ) {
        if (fe->isActive()) {
            // TODO this loads only FE-i4 specific stuff, bad
            // Load histogrammer
            histogrammers[fe].reset( new Fei4Histogrammer );
            auto& histogrammer = static_cast<Fei4Histogrammer&>( *(histogrammers[fe]) );

            histogrammer.connect(fe->clipData, fe->clipHisto);

            auto add_histo = [&](std::string algo_name) {
                if (algo_name == "OccupancyMap") {
                    logger->info("  ... adding {}", algo_name);
                    histogrammer.addHistogrammer(new OccupancyMap());
                } else if (algo_name == "TotMap") {
                    logger->info("  ... adding {}", algo_name);
                    histogrammer.addHistogrammer(new TotMap());
                } else if (algo_name == "Tot2Map") {
                    logger->info("  ... adding {}", algo_name);
                    histogrammer.addHistogrammer(new Tot2Map());
                } else if (algo_name == "L1Dist") {
                    histogrammer.addHistogrammer(new L1Dist());
                    logger->info("  ... adding {}", algo_name);
                } else if (algo_name == "HitsPerEvent") {
                    histogrammer.addHistogrammer(new HitsPerEvent());
                    logger->info("  ... adding {}", algo_name);
                } else if (algo_name == "DataArchiver") {
                    histogrammer.addHistogrammer(new DataArchiver((outputDir + dynamic_cast<FrontEndCfg*>(fe)->getName() + "_data.raw")));
                    logger->info("  ... adding {}", algo_name);
                } else if (algo_name == "Tot3d") {
                    logger->info("  ... adding {}", algo_name);
                    histogrammer.addHistogrammer(new Tot3d());
                } else if (algo_name == "L13d") {
                    logger->info("  ... adding {}", algo_name);
                    histogrammer.addHistogrammer(new L13d());
                } else {
                    std::cerr << "#ERROR# Histogrammer \"" << algo_name << "\" unknown, skipping!" << std::endl;
                }
            };

            try {
                int nHistos = histoCfg["n_count"];

                for (int j=0; j<nHistos; j++) {
                    std::string algo_name = histoCfg[std::to_string(j)]["algorithm"];
                    add_histo(algo_name);
                }
            } catch(/* json::type_error &te*/ ... ) { //FIXME
                int nHistos = histoCfg.size();
                for (int j=0; j<nHistos; j++) {
                    std::string algo_name = histoCfg[j]["algorithm"];
                    add_histo(algo_name);
                }
            }
            histogrammer.setMapSize(fe->geo.nCol, fe->geo.nRow);
        }
    }
}


void buildAnalyses( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& analyses, const std::string& scanType, Bookkeeper& bookie, ScanBase* s, int mask_opt) {
    if (scanType.find("json") != std::string::npos) {
        logger->info("Found Scan config, loading analysis ...");
        json scanCfg;
        try {
            scanCfg = ScanHelper::openJsonFile(scanType);
        } catch (std::runtime_error &e) {
            std::cerr << "#ERROR# opening scan config: " << e.what() << std::endl;
            throw("buildAnalyses failure");
        }
        json histoCfg = scanCfg["scan"]["histogrammer"];
        json anaCfg = scanCfg["scan"]["analysis"];

        for (FrontEnd *fe : bookie.feList ) {
            if (fe->isActive()) {
                // TODO this loads only FE-i4 specific stuff, bad
                // TODO hardcoded
                analyses[fe].reset( new Fei4Analysis(&bookie, dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()) );
                auto& ana = static_cast<Fei4Analysis&>( *(analyses[fe]) );
                ana.connect(s, fe->clipHisto, fe->clipResult);

                auto add_analysis = [&](std::string algo_name) {
                    if (algo_name == "OccupancyAnalysis") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new OccupancyAnalysis());
                     } else if (algo_name == "L1Analysis") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new L1Analysis());
                     } else if (algo_name == "TotAnalysis") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new TotAnalysis());
                     } else if (algo_name == "NoiseAnalysis") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new NoiseAnalysis());
                     } else if (algo_name == "NoiseTuning") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new NoiseTuning());
                     } else if (algo_name == "ScurveFitter") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new ScurveFitter());
                     } else if (algo_name == "OccGlobalThresholdTune") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new OccGlobalThresholdTune());
                     } else if (algo_name == "OccPixelThresholdTune") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new OccPixelThresholdTune());
                     } else if (algo_name == "DelayAnalysis") {
                        logger->info("  ... adding {}", algo_name);
                        ana.addAlgorithm(new DelayAnalysis());
                     }
                };

                try {
                  int nAnas = anaCfg["n_count"];
                  logger->info("Found {} Analysis!", nAnas);
                  for (int j=0; j<nAnas; j++) {
                    std::string algo_name = anaCfg[std::to_string(j)]["algorithm"];
                    add_analysis(algo_name);
                  }
                  ana.loadConfig(anaCfg);
                } catch(/* json::type_error &te */ ...) { //FIXME
                  int nAnas = anaCfg.size();
                  logger->info("Found {} Analysis!", nAnas);
                  for (int j=0; j<nAnas; j++) {
                    std::string algo_name = anaCfg[j]["algorithm"];
                    add_analysis(algo_name);
                  }
                }

                // Disable masking of pixels
                if(mask_opt == 0) {
                    logger->info("Disabling masking for this scan!");
                    ana.setMasking(false);
                }
                ana.setMapSize(fe->geo.nCol, fe->geo.nRow);
            }
        }
    } 
}

void setupLoggers(const json &j) {
    spdlog::sink_ptr default_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // spdlog::level::level_enum, but then construction doesn't work?
    const std::map<std::string, int> level_map = {
      {"off", SPDLOG_LEVEL_OFF},
      {"critical", SPDLOG_LEVEL_CRITICAL},
      {"err", SPDLOG_LEVEL_ERROR},
      {"error", SPDLOG_LEVEL_ERROR},
      {"warn", SPDLOG_LEVEL_WARN},
      {"warning", SPDLOG_LEVEL_WARN},
      {"info", SPDLOG_LEVEL_INFO},
      {"debug", SPDLOG_LEVEL_DEBUG},
      {"trace", SPDLOG_LEVEL_TRACE},
    };

    if(!j["simple"].empty()) {
        // Don't print log level and timestamp
        if (j["simple"])
            default_sink->set_pattern("%v");
    }

    if(!j["log_config"].empty()) {
        for(auto &jl: j["log_config"]) {
            if(jl["name"].empty()) {
                std::cerr << "Log json file: 'log_config' list item must have 'name'\n";
                continue;
            }

            std::string name = jl["name"];

            auto logger_apply = [&](std::shared_ptr<spdlog::logger> l) {
              l->sinks().push_back(default_sink);
              if(jl["level"].empty()) {
                  std::string level = jl["level"];

                  spdlog::level::level_enum spd_level = (spdlog::level::level_enum)level_map.at(level);
                  l->set_level(spd_level);
              }
            };

            if(name == "all") {
                spdlog::apply_all(logger_apply);
            } else {
                logger_apply(spdlog::get(name));
            }
        }
    }

    // NB this sets things at the sink level, so not specifying a particular logger...
    // Also doing it last means it applies to all registered sinks
    if(j["pattern"].empty()) {
        std::string pattern = j["pattern"];
      
        spdlog::set_pattern(pattern);
    }
}
