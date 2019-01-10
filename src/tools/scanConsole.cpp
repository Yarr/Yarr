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

#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)

//  #include <errno.h>
//  #include <sys/stat.h>
#include <cstdlib> //I am not proud of this ):

#endif

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

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


int main(int argc, char *argv[]) {
    std::cout << "\033[1;31m#####################################\033[0m" << std::endl;
    std::cout << "\033[1;31m# Welcome to the YARR Scan Console! #\033[0m" << std::endl;
    std::cout << "\033[1;31m#####################################\033[0m" << std::endl;

    std::cout << "\033[1;-> Parsing command line parameters ..." << std::endl;
    
    // Init parameters
    std::string scanType = "";
    std::vector<std::string> cConfigPaths;
    std::string outputDir = "./data/";
    std::string ctrlCfgPath = "";
    bool doPlots = false;
    int target_charge = -1;
    int target_tot = -1;
    int mask_opt = -1;
    
    unsigned runCounter = 0;

    // Load run counter
    if (system("mkdir -p ~/.yarr") < 0) {
        std::cerr << "#ERROR# Loading run counter ~/.yarr!" << std::endl;
    }
    
    std::string home = getenv("HOME");
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

    int c;
    while ((c = getopt(argc, argv, "hks:n:m:g:r:c:t:po:")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
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

    // Timestamp
    std::time_t now = std::time(NULL);
    struct tm *lt = std::localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%F_%H:%M:%S", lt);
    std::cout << std::endl;
    std::cout << "Timestamp: " << timestamp << std::endl;
    std::cout << "Run Number: " << runCounter;

    json scanLog;
    // Add to scan log
    scanLog["timestamp"] = timestamp;
    scanLog["runNumber"] = runCounter;
    scanLog["targetCharge"] = target_charge;
    scanLog["targetTot"] = target_tot;

    std::cout << std::endl;
    std::cout << "\033[1;31m#################\033[0m" << std::endl;
    std::cout << "\033[1;31m# Init Hardware #\033[0m" << std::endl;
    std::cout << "\033[1;31m#################\033[0m" << std::endl;

    std::unique_ptr<HwController> hwCtrl = nullptr;
    if (ctrlCfgPath == "") {
        std::cout << "#ERRROR# No controller config given, aborting." << std::endl;
        return -1;
    } else {
        // Open controller config file
        std::cout << "-> Opening controller config: " << ctrlCfgPath << std::endl;
        std::ifstream ctrlCfgFile(ctrlCfgPath);
        if (!ctrlCfgFile) {
            std::cerr <<"#ERROR# Cannot open controller config file: " << ctrlCfgPath << std::endl;
            return -1;
        }
        json ctrlCfg;
        try {
            ctrlCfg = json::parse(ctrlCfgFile);
        } catch (json::parse_error &e) {
            std::cerr << "#ERROR# Could not parse config: " << e.what() << std::endl;
            return 0;
        }
        std::string controller = ctrlCfg["ctrlCfg"]["type"];
        // Add to scan log
        scanLog["ctrlCfg"] = ctrlCfg;

        hwCtrl = StdDict::getHwController(controller);

        if(hwCtrl) {
          std::cout << "-> Found config for controller " << controller << std::endl;

          hwCtrl->loadConfig(ctrlCfg["ctrlCfg"]["cfg"]);
        } else {
            std::cerr << "#ERROR# Unknown config type: " << ctrlCfg["ctrlCfg"]["type"] << std::endl;
            std::cout << " Known HW controllers:\n";
            for(auto &h: StdDict::listHwControllers()) {
              std::cout << "  " << h << std::endl;
            }
            std::cerr << "Aborting!" << std::endl;
            return -1;
        }
    }
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
        std::cout << "Opening global config: " << sTmp << std::endl;
        std::ifstream gConfig(sTmp);
        json config;
        try {
            config = json::parse(gConfig);
        } catch(json::parse_error &e) {
            std::cerr << __PRETTY_FUNCTION__ << " : " << e.what() << std::endl;
        }
        scanLog["connectivity"] = config;

        if (config["chipType"].empty() || config["chips"].empty()) {
            std::cerr << __PRETTY_FUNCTION__ << " : invalid config, chip type or chips not specified!" << std::endl;
            return 0;
        } else {
            chipType = config["chipType"];
            std::cout << "Chip Type: " << chipType << std::endl;
            std::cout << "Found " << config["chips"].size() << " chips defined!" << std::endl;
            // Loop over chips
            for (unsigned i=0; i<config["chips"].size(); i++) {
                std::cout << "Loading chip #" << i << std::endl;
                try { 
                    json chip = config["chips"][i];
                    std::string chipConfigPath = chip["config"];
                    // TODO should be a shared pointer
                    bookie.addFe(StdDict::getFrontEnd(chipType).release(), chip["tx"], chip["rx"]);
                    bookie.getLastFe()->init(&*hwCtrl, chip["tx"], chip["rx"]);
                    std::ifstream cfgFile(chipConfigPath);
                    if (cfgFile) {
                        // Load config
                        std::cout << "Loading config file: " << chipConfigPath << std::endl;
                        json cfg = json::parse(cfgFile);
                        dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->fromFileJson(cfg);
                        if (!chip["locked"].empty())
                            dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->setLocked(chip["locked"]);
                        cfgFile.close();
                    } else {
                        std::cout << "Config file not found, using default!" << std::endl;
                    }
                    success++;
                    // Save path to config
                    std::size_t botDirPos = chipConfigPath.find_last_of("/");
                    feCfgMap[bookie.getLastFe()] = chipConfigPath;
                    dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->setConfigFile(chipConfigPath.substr(botDirPos, chipConfigPath.length()));
                    
                    // Create backup of current config
                    // TODO fix folder
                    std::ofstream backupCfgFile(outputDir + dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->getConfigFile() + ".before");
                    json backupCfg;
                    dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->toFileJson(backupCfg);
                    backupCfgFile << std::setw(4) << backupCfg;
                    backupCfgFile.close();
                    
                } catch (json::parse_error &e) {
                    std::cerr << __PRETTY_FUNCTION__ << " : " << e.what() << std::endl;
                }
            }
        }
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
        std::cout << "-> Configuring " << dynamic_cast<FrontEndCfg*>(fe)->getName() << std::endl;
        // Select correct channel
        hwCtrl->setCmdEnable(0x1 << dynamic_cast<FrontEndCfg*>(fe)->getTxChannel());
        // Configure
        fe->configure();
        // Wait for fifo to be empty
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        while(!hwCtrl->isCmdEmpty());
    }
    std::chrono::steady_clock::time_point cfg_end = std::chrono::steady_clock::now();
    std::cout << "-> All FEs configured in " 
        << std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count() << " ms !" << std::endl;
    
    // Wait for rx to sync with FE stream
    // TODO Check RX sync
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    // Enable all active channels
    hwCtrl->setCmdEnable(bookie.getTxMask());
    std::cout << "-> Setting Tx Mask to: 0x" << std::hex << bookie.getTxMask() << std::dec << std::endl;
    hwCtrl->setRxEnable(bookie.getRxMask());
    std::cout << "-> Setting Rx Mask to: 0x" << std::hex << bookie.getRxMask() << std::dec << std::endl;
    
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

    std::cout << "-> Running pre scan!" << std::endl;
    s->init();
    s->preScan();

    // Run from downstream to upstream
    std::cout << "-> Starting histogrammer and analysis threads:" << std::endl;
    for ( FrontEnd* fe : bookie.feList ) {
        if (fe->isActive()) {
          analyses[fe]->init();
          analyses[fe]->run();
          
          histogrammers[fe]->init();
          histogrammers[fe]->run();
          
          std::cout << "  -> Analysis thread of Fe " << dynamic_cast<FrontEndCfg*>(fe)->getRxChannel() << std::endl;
        }
    }

    std::shared_ptr<DataProcessor> proc = StdDict::getDataProcessor(chipType);
    //Fei4DataProcessor proc(bookie.globalFe<Fei4>()->getValue(&Fei4::HitDiscCnfg));
    proc->connect( &bookie.rawData, &bookie.eventMap );
    proc->init();
    proc->run();

    // Now the all downstream processors are ready --> Run scan

    std::cout << std::endl;
    std::cout << "\033[1;31m########\033[0m" << std::endl;
    std::cout << "\033[1;31m# Scan #\033[0m" << std::endl;
    std::cout << "\033[1;31m########\033[0m" << std::endl;

    std::cout << "-> Starting scan!" << std::endl;
    std::chrono::steady_clock::time_point scan_start = std::chrono::steady_clock::now();
    s->run();
    s->postScan();
    std::cout << "-> Scan done!" << std::endl;

    // Join from upstream to downstream.
    
    proc->scanDone = true;
    bookie.rawData.cv.notify_all();

    std::chrono::steady_clock::time_point scan_done = std::chrono::steady_clock::now();
    std::cout << "-> Waiting for processors to finish ..." << std::endl;
    // Join Fei4DataProcessor
    proc->join();
    std::chrono::steady_clock::time_point processor_done = std::chrono::steady_clock::now();
    
    std::cout << "-> Processor done, waiting for histogrammer ..." << std::endl;
    
    Fei4Histogrammer::processorDone = true;
    
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        if (fe->isActive()) {
          fe->clipData->cv.notify_all();
        }
    }
    
    // Join histogrammers
    for( auto& histogrammer : histogrammers ) {
      histogrammer.second->join();
    }
    
    std::cout << "-> Processor done, waiting for analysis ..." << std::endl;
    
    Fei4Analysis::histogrammerDone = true;
    
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        if (fe->isActive()) {
          fe->clipHisto->cv.notify_all();
        }
    }

    // Join analyses
    for( auto& ana : analyses ) {
      ana.second->join();
    }
      
    std::chrono::steady_clock::time_point all_done = std::chrono::steady_clock::now();
    std::cout << "-> All done!" << std::endl;

    // Joining is done.

    //hwCtrl->setCmdEnable(0x0);
    hwCtrl->setRxEnable(0x0);

    std::cout << std::endl;
    std::cout << "\033[1;31m##########\033[0m" << std::endl;
    std::cout << "\033[1;31m# Timing #\033[0m" << std::endl;
    std::cout << "\033[1;31m##########\033[0m" << std::endl;

    std::cout << "-> Configuration: " << std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count() << " ms" << std::endl;
    std::cout << "-> Scan:          " << std::chrono::duration_cast<std::chrono::milliseconds>(scan_done-scan_start).count() << " ms" << std::endl;
    std::cout << "-> Processing:    " << std::chrono::duration_cast<std::chrono::milliseconds>(processor_done-scan_done).count() << " ms" << std::endl;
    std::cout << "-> Analysis:      " << std::chrono::duration_cast<std::chrono::milliseconds>(all_done-processor_done).count() << " ms" << std::endl;

    std::cout << std::endl;
    std::cout << "\033[1;31m###########\033[0m" << std::endl;
    std::cout << "\033[1;31m# Cleanup #\033[0m" << std::endl;
    std::cout << "\033[1;31m###########\033[0m" << std::endl;

    // Call constructor (eg shutdown Emu threads)
    hwCtrl.reset();

    // Save scan log
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
                std::cout << "-> Saving config of FE " << dynamic_cast<FrontEndCfg*>(fe)->getName() << " to " << feCfgMap.at(fe) << std::endl;
                json jTmp;
                dynamic_cast<FrontEndCfg*>(fe)->toFileJson(jTmp);
                std::ofstream oFTmp(feCfgMap.at(fe));
                oFTmp << std::setw(4) << jTmp;
                oFTmp.close();
            } else {
                std::cout << "Not saving config for FE " << dynamic_cast<FrontEndCfg*>(fe)->getName() << " as it is protected!" << std::endl;
            }

            // Save extra config in data folder
            std::ofstream backupCfgFile(outputDir + dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->getConfigFile() + ".after");
            json backupCfg;
            dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->toFileJson(backupCfg);
            backupCfgFile << std::setw(4) << backupCfg;
            backupCfgFile.close(); 

            // Plot
            if (doPlots) {
                std::cout << "-> Plotting histograms of FE " << dynamic_cast<FrontEndCfg*>(fe)->getRxChannel() << std::endl;
                std::string outputDirTmp = outputDir;

                auto &output = *fe->clipResult;
                std::string name = dynamic_cast<FrontEndCfg*>(fe)->getName();

                while(!output.empty()) {
                    std::unique_ptr<HistogramBase> histo = output.popData();
                    histo->plot(name, outputDirTmp);
                    histo->toFile(name, outputDir);
                }
            }
        }
    }
    std::string lsCmd = "ls -1 " + dataDir + "last_scan/*.p*";
    if (system(lsCmd.c_str()) < 0) {
        std::cout << "Find plots in: " << dataDir + "last_scan" << std::endl;
    }
    return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -s <scan_type> : Scan config" << std::endl;
    //std::cout << " -n: Provide SPECboard number." << std::endl;
    //std::cout << " -g <cfg_list.txt>: Provide list of chip configurations." << std::endl;
    std::cout << " -c <cfg1.json> [<cfg2.json> ...]: Provide connectivity configuration, can take multiple arguments." << std::endl;
    std::cout << " -r <ctrl.json> Provide controller configuration." << std::endl;
    std::cout << " -t <target_charge> [<tot_target>] : Set target values for threshold/charge (and tot)." << std::endl;
    std::cout << " -p: Enable plotting of results." << std::endl;
    std::cout << " -o <dir> : Output directory. (Default ./data/)" << std::endl;
    std::cout << " -m <int> : 0 = disable pixel masking, 1 = reset pixel masking, default = enable pixel masking" << std::endl;
    std::cout << " -k: Report known items (Scans, Hardware etc.)\n";
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
}

std::unique_ptr<ScanBase> buildScan( const std::string& scanType, Bookkeeper& bookie ) {
  std::unique_ptr<ScanBase> s ( nullptr );
  
    if (scanType.find("json") != std::string::npos) {
        std::cout << "-> Found Scan config, constructing scan ..." << std::endl;
        s.reset( new ScanFactory(&bookie) );
        std::ifstream scanCfgFile(scanType);
        if (!scanCfgFile) {
            std::cerr << "#ERROR# Could not open scan config: " << scanType << std::endl;
            throw("buildScan failure!");
        }
        json scanCfg;
        try {
            scanCfg = json::parse(scanCfgFile);
        } catch (json::parse_error &e) {
            std::cerr << "#ERROR# Could not parse config: " << e.what() << std::endl;
        }
        dynamic_cast<ScanFactory&>(*s).loadConfig(scanCfg);
    } else {
        std::cout << "-> Selecting Scan: " << scanType << std::endl;
        auto scan = StdDict::getScan(scanType, &bookie);
        if (scan != nullptr) {
            std::cout << "-> Found Scan for " << scanType << std::endl;
            s = std::move(scan);
        } else {
            std::cout << "-> No matching Scan found, possible:" << std::endl;
            listScans();
            std::cerr << "-> Aborting!" << std::endl;
            throw("buildScan failure!");
        }
    }

    return s;
}


void buildHistogrammers( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& histogrammers, const std::string& scanType, std::vector<FrontEnd*>& feList, ScanBase* s, std::string outputDir) {
    if (scanType.find("json") != std::string::npos) {
        std::cout << "-> Found Scan config, loading histogrammer ..." << std::endl;
        std::ifstream scanCfgFile(scanType);
        if (!scanCfgFile) {
            std::cerr << "#ERROR# Could not open scan config: " << scanType << std::endl;
            throw("buildHistogrammers failure!");
        }
        json scanCfg;
        scanCfg= json::parse(scanCfgFile);
        json histoCfg = scanCfg["scan"]["histogrammer"];
        json anaCfg = scanCfg["scan"]["analysis"];

        for (FrontEnd *fe : feList ) {
            if (fe->isActive()) {
                // TODO this loads only FE-i4 specific stuff, bad
                // Load histogrammer
                histogrammers[fe].reset( new Fei4Histogrammer );
                auto& histogrammer = static_cast<Fei4Histogrammer&>( *(histogrammers[fe]) );
                
                histogrammer.connect(fe->clipData, fe->clipHisto);
                int nHistos = histoCfg["n_count"];
                std::cout << nHistos << std::endl;
                for (int j=0; j<nHistos; j++) {
                    std::string algo_name = histoCfg[std::to_string(j)]["algorithm"];
                    if (algo_name == "OccupancyMap") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        histogrammer.addHistogrammer(new OccupancyMap());
                    } else if (algo_name == "TotMap") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        histogrammer.addHistogrammer(new TotMap());
                    } else if (algo_name == "Tot2Map") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        histogrammer.addHistogrammer(new Tot2Map());
                    } else if (algo_name == "L1Dist") {
                        histogrammer.addHistogrammer(new L1Dist());
                        std::cout << "  ... adding " << algo_name << std::endl;
                    } else if (algo_name == "HitsPerEvent") {
                        histogrammer.addHistogrammer(new HitsPerEvent());
                        std::cout << "  ... adding " << algo_name << std::endl;
                    } else if (algo_name == "DataArchiver") {
                        histogrammer.addHistogrammer(new DataArchiver((outputDir + "data.raw")));
                        std::cout << "  ... adding " << algo_name << std::endl;
                    } else if (algo_name == "Tot3d") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        histogrammer.addHistogrammer(new Tot3d());
                    } else if (algo_name == "L13d") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        histogrammer.addHistogrammer(new L13d());
                    } else {
                        std::cerr << "#ERROR# Histogrammer \"" << algo_name << "\" unknown, skipping!" << std::endl;
                    }
                }
                histogrammer.setMapSize(fe->geo.nCol, fe->geo.nRow);
            }
        }
    } else {
        // Init histogrammer and analysis
      for (FrontEnd *fe : feList ) {
            if (fe->isActive()) {
                // Init histogrammer per FE
                histogrammers[fe].reset( new Fei4Histogrammer );
                auto& histogrammer = static_cast<Fei4Histogrammer&>( *(histogrammers[fe]) );
                
                histogrammer.connect(fe->clipData, fe->clipHisto);
                // Add generic histograms
                histogrammer.addHistogrammer(new OccupancyMap());
                histogrammer.addHistogrammer(new TotMap());
                histogrammer.addHistogrammer(new Tot2Map());
                histogrammer.addHistogrammer(new L1Dist());
                histogrammer.addHistogrammer(new HitsPerEvent());
                if (scanType == "selftrigger") {
                    // TODO set proper file name
                    histogrammer.addHistogrammer(new DataArchiver((outputDir + "data.raw")));
                }
                histogrammer.setMapSize(fe->geo.nCol, fe->geo.nRow);
            }
        }
    }
}


void buildAnalyses( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& analyses, const std::string& scanType, Bookkeeper& bookie, ScanBase* s, int mask_opt) {
    if (scanType.find("json") != std::string::npos) {
        std::cout << "-> Found Scan config, loading analysis ..." << std::endl;
        std::ifstream scanCfgFile(scanType);
        if (!scanCfgFile) {
            std::cerr << "#ERROR# Could not open scan config: " << scanType << std::endl;
            throw( "buildAnalyses failed" );
        }
        json scanCfg;
        scanCfg = json::parse(scanCfgFile);
        json histoCfg = scanCfg["scan"]["histogrammer"];
        json anaCfg = scanCfg["scan"]["analysis"];

        for (FrontEnd *fe : bookie.feList ) {
            if (fe->isActive()) {
                // TODO this loads only FE-i4 specific stuff, bad
                // TODO hardcoded
                analyses[fe].reset( new Fei4Analysis(&bookie, dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()) );
                auto& ana = static_cast<Fei4Analysis&>( *(analyses[fe]) );
                ana.connect(s, fe->clipHisto, fe->clipResult);
                
                int nAnas = anaCfg["n_count"];
                std::cout << "Found " << nAnas << " Analysis!" << std::endl;
                for (int j=0; j<nAnas; j++) {
                    std::string algo_name = anaCfg[std::to_string(j)]["algorithm"];
                    if (algo_name == "OccupancyAnalysis") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        ana.addAlgorithm(new OccupancyAnalysis());
                     } else if (algo_name == "L1Analysis") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        ana.addAlgorithm(new L1Analysis());
                     } else if (algo_name == "TotAnalysis") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        ana.addAlgorithm(new TotAnalysis());
                     } else if (algo_name == "NoiseAnalysis") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        ana.addAlgorithm(new NoiseAnalysis());
                     } else if (algo_name == "NoiseTuning") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        ana.addAlgorithm(new NoiseTuning());
                     } else if (algo_name == "ScurveFitter") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        ana.addAlgorithm(new ScurveFitter());
                     } else if (algo_name == "OccGlobalThresholdTune") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        ana.addAlgorithm(new OccGlobalThresholdTune());
                     } else if (algo_name == "OccPixelThresholdTune") {
                        std::cout << "  ... adding " << algo_name << std::endl;
                        ana.addAlgorithm(new OccPixelThresholdTune());
                     }

                }
                // Disable masking of pixels
                if(mask_opt == 0) {
                    std::cout << " -> Disabling masking for this scan!" << std::endl;
                    ana.setMasking(false);
                }
                ana.setMapSize(fe->geo.nCol, fe->geo.nRow);
            }
        }
    } else {
        // Init histogrammer and analysis
      for (FrontEnd *fe : bookie.feList ) {
            if (fe->isActive()) {
                // Init analysis per FE and depending on scan type
                analyses[fe].reset( new Fei4Analysis(&bookie, dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()) );
                auto& ana = static_cast<Fei4Analysis&>( *(analyses[fe]) );
                ana.connect(s, fe->clipHisto, fe->clipResult);
                ana.addAlgorithm(new L1Analysis());
                if (scanType == "digitalscan") {
                    ana.addAlgorithm(new OccupancyAnalysis());
                } else if (scanType == "analogscan") {
                    ana.addAlgorithm(new OccupancyAnalysis());
                } else if (scanType == "thresholdscan") {
                    ana.addAlgorithm(new ScurveFitter());
                } else if (scanType == "totscan") {
                    ana.addAlgorithm(new TotAnalysis());
                } else if (scanType == "tune_globalthreshold") {
                    ana.addAlgorithm(new OccGlobalThresholdTune());
                } else if (scanType == "tune_pixelthreshold") {
                    ana.addAlgorithm(new OccPixelThresholdTune());
                } else if (scanType == "tune_globalpreamp") {
                    ana.addAlgorithm(new TotAnalysis());
                } else if (scanType == "tune_pixelpreamp") {
                    ana.addAlgorithm(new TotAnalysis());
                } else if (scanType == "noisescan") {
                    ana.addAlgorithm(new NoiseAnalysis());
                } else if (scanType == "selftrigger") {
                    ana.addAlgorithm(new OccupancyAnalysis());
                    ana.getLastAna()->disMasking();
                } else if (scanType == "selftrigger_noise") {
                    ana.addAlgorithm(new NoiseAnalysis());
                } else {
                    std::cout << "-> Analyses not defined for scan type" << std::endl;
                    listScans();
                    std::cerr << "-> Aborting!" << std::endl;
                    throw("buildAnalyses failure!");
                }
                // Disable masking of pixels
                if(mask_opt == 0) {
                    std::cout << " -> Disabling masking for this scan!" << std::endl;
                    ana.setMasking(false);
                }
                ana.setMapSize(fe->geo.nCol, fe->geo.nRow);
            }
        }
    }
}
