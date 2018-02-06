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

#include "Bookkeeper.h"
#include "Fei4.h"
#include "ScanBase.h"
#include "ScanFactory.h"
#include "Fei4DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "Fei4Analysis.h"
#include "Fei4Scans.h"

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

std::unique_ptr<ScanBase> buildScan( const std::string& scanType, Bookkeeper& bookie );

// In order to build Histogrammer, bookie is not needed --> good sign!
// Do not want to use the raw pointer ScanBase*
void buildHistogrammers( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& histogrammers, const std::string& scanType, std::vector<FrontEnd*>& feList, ScanBase* s, std::string outputDir);

// In order to build Analysis, bookie is needed --> deep dependency!
// Do not want to use the raw pointer ScanBase*
void buildAnalyses( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& analyses, const std::string& scanType, Bookkeeper& bookie, ScanBase* s, int mask_opt);


int main(int argc, char *argv[]) {
    std::cout << "#####################################" << std::endl;
    std::cout << "# Welcome to the YARR Scan Console! #" << std::endl;
    std::cout << "#####################################" << std::endl;

    std::cout << "-> Parsing command line parameters ..." << std::endl;
    
    // Init parameters
    unsigned specNum = 0;
    std::string scanType = "";
    std::vector<std::string> cConfigPaths;
    std::string outputDir = "./data/";
    std::string ctrlCfgPath = "";
    bool doPlots = false;
    int target_threshold = 2500;
    int target_tot = 10;
    int target_charge = 16000;
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
        system("echo \"1\n\" > ~/.yarr/runCounter");
        runCounter = 1;
    }
    iF.close();

    std::fstream oF((home + "/.yarr/runCounter").c_str(), std::ios::out);
    oF << runCounter << std::endl;
    oF.close();

    int c;
    while ((c = getopt(argc, argv, "hs:n:m:g:r:c:t:po:")) != -1) {
        int count = 0;
        switch (c) {
            case 'h':
                printHelp();
                return 0;
                break;
            case 's':
                scanType = std::string(optarg);
                break;
            case 'n':
                specNum = atoi(optarg);
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
                            target_threshold = atoi(argv[optind]);
                            break;
                        case 1:
                            target_tot = atoi(argv[optind]);
                            break;
                        case 2:
                            target_charge = atoi(argv[optind]);
                            break;
                        default:
                            std::cerr << "-> Can only receive max. 3 parameters with -t!!" << std::endl;
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

    outputDir += (toString(runCounter, 6) + "_" + scanType + "/");
    
    std::cout << " SPEC Nr: " << specNum << std::endl;
    std::cout << " Scan Type: " << scanType << std::endl;
    
    std::cout << " Chips: " << std::endl;
    for(std::string const& sTmp : cConfigPaths){
        std::cout << "    " << sTmp << std::endl;
    }
    std::cout << " Target Threshold: " << target_threshold << std::endl;
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

    // Timestamp
    std::time_t now = std::time(NULL);
    struct tm *lt = std::localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%F_%H:%M:%S", lt);
    std::cout << std::endl;
    std::cout << "Timestamp: " << timestamp << std::endl;
    std::cout << "Run Number: " << runCounter;

    std::cout << std::endl;
    std::cout << "#################" << std::endl;
    std::cout << "# Init Hardware #" << std::endl;
    std::cout << "#################" << std::endl;

    std::unique_ptr<HwController> hwCtrl = nullptr;
    if (ctrlCfgPath == "") {
        std::cout << "-> No controller config given, using default." << std::endl;
        std::cout << "-> Init SPEC " << specNum << " : " << std::endl;
        hwCtrl = StdDict::getHwController("Spec"); // TODO fix me with specnum
    } else {
        // Open controller config file
        std::cout << "-> Opening controller config: " << ctrlCfgPath << std::endl;
        std::ifstream ctrlCfgFile(ctrlCfgPath);
        if (!ctrlCfgFile) {
            std::cerr <<"#ERROR# Cannot open controller config file: " << ctrlCfgPath << std::endl;
            return -1;
        }
        json ctrlCfg;
        ctrlCfg << ctrlCfgFile;
        std::string controller = ctrlCfg["ctrlCfg"]["type"];

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

    Bookkeeper bookie(&*hwCtrl, &*hwCtrl);
    bookie.initGlobalFe(new Fei4(&*hwCtrl)); // TODO dynamic depending on current FE type

    std::map<FrontEnd*, std::string> feCfgMap;

    bookie.setTargetThreshold(target_threshold);
    bookie.setTargetTot(target_tot);
    bookie.setTargetCharge(target_charge);

    std::cout << "#######################" << std::endl
              << "##  Loading Configs  ##" << std::endl
              << "#######################" << std::endl;
    
    for(std::string const& sTmp : cConfigPaths){
        std::string discardMe; //Error handling, wait for user
        json jTmp;
        std::ifstream iFTmp(sTmp);

        if (!iFTmp) {
            std::cerr << "File not found: " << sTmp << std::endl;
            std::cerr << "Creating new config (type FE-I4B) ..." << std::endl;
            unsigned i=0;
            for (; i<16; i++) {
                if (bookie.getFe(i) == NULL)
                    break;
            }
            std::cout << "Rx " << i << " seems to be free, assuming same Tx channel." << std::endl;
            bookie.addFe(dynamic_cast<FrontEnd*>(new Fei4(&*hwCtrl)), i, i);
        } else {
            jTmp << iFTmp;
            if(!jTmp["FE-I4B"].is_null()){
                std::cout << "Found FE-I4B: " << jTmp["FE-I4B"]["name"] << std::endl;
                bookie.addFe(dynamic_cast<FrontEnd*>(new Fei4(&*hwCtrl)), jTmp["FE-I4B"]["txChannel"], jTmp["FE-I4B"]["rxChannel"]);        
            } else if(!jTmp["FE65-P2"].is_null()){
                std::cout << "Found FE65-P2: " << jTmp["FE-I4B"]["name"] << std::endl;
                bookie.addFe(dynamic_cast<FrontEnd*>(new Fe65p2(&*hwCtrl)), jTmp["FE65-P2"]["txChannel"], jTmp["FE-I4B"]["rxChannel"]);        
            } else{
                std::cerr << "Unknown chip type or malformed config in " << sTmp << std::endl;
                continue;
            }
            // Load config
            dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->fromFileJson(jTmp);
            // Reset mask
            if (mask_opt == 1) {
                if (!jTmp["FE-I4B"].is_null()) {
                    std::cout << "Resetting enable/hitbus pixel mask to all enabled!" << std::endl;
                    Fei4 *fe = dynamic_cast<Fei4*>(bookie.getLastFe());
                    for (unsigned int dc = 0; dc < fe->n_DC; dc++) {
                        fe->En(dc).setAll(1);
                        fe->Hitbus(dc).setAll(0);
                    }
                }
                // TODO add FE65p2
            }
                
        }
        feCfgMap[bookie.getLastFe()] = sTmp;
        iFTmp.close();
        
        // Create backup of current config
        std::ofstream backupCfgFile(outputDir + dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->getName() + ".json.after");
        json backupCfg;
        dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->toFileJson(backupCfg);
        backupCfgFile << std::setw(4) << backupCfg;
        backupCfgFile.close();
    }
    
    std::cout << std::endl;
    std::cout << "#################" << std::endl;
    std::cout << "# Configure FEs #" << std::endl;
    std::cout << "#################" << std::endl;
    
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
    
    std::cout << std::endl;
    std::cout << "##############" << std::endl;
    std::cout << "# Setup Scan #" << std::endl;
    std::cout << "##############" << std::endl;

    // TODO Make this nice
    std::unique_ptr<ScanBase> s = buildScan(scanType, bookie );

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

    Fei4DataProcessor proc(bookie.globalFe<Fei4>()->getValue(&Fei4::HitDiscCnfg));
    proc.connect( &bookie.rawData, &bookie.eventMap );
    proc.init();
    proc.run();

    // Now the all downstream processors are ready --> Run scan

    std::cout << std::endl;
    std::cout << "########" << std::endl;
    std::cout << "# Scan #" << std::endl;
    std::cout << "########" << std::endl;

    std::cout << "-> Starting scan!" << std::endl;
    std::chrono::steady_clock::time_point scan_start = std::chrono::steady_clock::now();
    s->run();
    s->postScan();
    std::cout << "-> Scan done!" << std::endl;

    // Join from upstream to downstream.
    
    Fei4DataProcessor::scanDone = true;
    bookie.rawData.cv.notify_all();

    std::chrono::steady_clock::time_point scan_done = std::chrono::steady_clock::now();
    std::cout << "-> Waiting for processors to finish ..." << std::endl;
    // Join Fei4DataProcessor
    proc.join();
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

    hwCtrl->setCmdEnable(0x0);
    hwCtrl->setRxEnable(0x0);

    std::cout << std::endl;
    std::cout << "##########" << std::endl;
    std::cout << "# Timing #" << std::endl;
    std::cout << "##########" << std::endl;

    std::cout << "-> Configuration: " << std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count() << " ms" << std::endl;
    std::cout << "-> Scan:          " << std::chrono::duration_cast<std::chrono::milliseconds>(scan_done-scan_start).count() << " ms" << std::endl;
    std::cout << "-> Processing:    " << std::chrono::duration_cast<std::chrono::milliseconds>(processor_done-scan_done).count() << " ms" << std::endl;
    std::cout << "-> Analysis:      " << std::chrono::duration_cast<std::chrono::milliseconds>(all_done-processor_done).count() << " ms" << std::endl;

    std::cout << std::endl;
    std::cout << "###########" << std::endl;
    std::cout << "# Cleanup #" << std::endl;
    std::cout << "###########" << std::endl;

    // Call constructor (eg shutdown Emu threads)
    hwCtrl.reset();

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
            std::cout << "-> Saving config of FE " << dynamic_cast<FrontEndCfg*>(fe)->getName() << " to " << feCfgMap.at(fe) << std::endl;
            json jTmp;
            dynamic_cast<FrontEndCfg*>(fe)->toFileJson(jTmp);
            std::ofstream oFTmp(feCfgMap.at(fe));
            oFTmp << std::setw(4) << jTmp;
            oFTmp.close();

            // Save extra config in data folder
            std::ofstream backupCfgFile(outputDir + dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->getName() + ".json.after");
            json backupCfg;
            dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->toFileJson(backupCfg);
            backupCfgFile << std::setw(4) << backupCfg;
            backupCfgFile.close(); 

            // Plot
            if (doPlots) {
                std::cout << "-> Plotting histograms of FE " << dynamic_cast<FrontEndCfg*>(fe)->getRxChannel() << std::endl;
                std::string outputDirTmp = outputDir;
                auto& ana = static_cast<Fei4Analysis&>( *(analyses[fe]) );
                ana.plot(dynamic_cast<FrontEndCfg*>(fe)->getName(), outputDirTmp);
                ana.toFile(dynamic_cast<FrontEndCfg*>(fe)->getName(), outputDir);
            }
        }
    }
    std::string lsCmd = "ls -1 " + outputDir + "*.p*";
    if (system(lsCmd.c_str()) < 0) {
        std::cout << "Find plots in: " << outputDir << std::endl;
    }
    return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -s <scan_type> : Scan type. Possible types:" << std::endl;
    listScans();
    //std::cout << " -n: Provide SPECboard number." << std::endl;
    //std::cout << " -g <cfg_list.txt>: Provide list of chip configurations." << std::endl;
    std::cout << " -c <cfg1.json> [<cfg2.json> ...]: Provide chip configuration, can take multiple arguments." << std::endl;
    std::cout << " -r <ctrl.json> Provide controller configuration." << std::endl;
    std::cout << " -t <target_threshold> [<tot_target> [<charge_target>]] : Set target values for threshold, tot, charge." << std::endl;
    std::cout << " -p: Enable plotting of results." << std::endl;
    std::cout << " -o <dir> : Output directory. (Default ./data/)" << std::endl;
    std::cout << " -m <int> : 0 = disable pixel masking, 1 = reset pixel masking, default = enable pixel masking" << std::endl;
}

void listScans() {
    std::cout << "  digitalscan" << std::endl;
    std::cout << "  analogscan" << std::endl;
    std::cout << "  thresholdscan" << std::endl;
    std::cout << "  totscan" << std::endl;
    std::cout << "  tune_globalthreshold" << std::endl;
    std::cout << "  tune_pixelthreshold" << std::endl;
    std::cout << "  tune_globalpreamp" << std::endl;
    std::cout << "  tune_pixelpreamp" << std::endl;
    std::cout << "  noisescan" << std::endl;
    std::cout << "  selftrigger" << std::endl;
    std::cout << "  selftrigger_noise" << std::endl;
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
        scanCfg << scanCfgFile;
        dynamic_cast<ScanFactory&>(*s).loadConfig(scanCfg);
    } else {
        std::cout << "-> Selecting Scan: " << scanType << std::endl;
        if (scanType == "digitalscan") {
            std::cout << "-> Found Digital Scan" << std::endl;
            s.reset( new Fei4DigitalScan(&bookie) );
        } else if (scanType == "analogscan") {
            std::cout << "-> Found Analog Scan" << std::endl;
            s.reset( new Fei4AnalogScan(&bookie) );
        } else if (scanType == "thresholdscan") {
            std::cout << "-> Found Threshold Scan" << std::endl;
            s.reset( new Fei4ThresholdScan(&bookie) );
        } else if (scanType == "totscan") {
            std::cout << "-> Found ToT Scan" << std::endl;
            s.reset( new Fei4TotScan(&bookie) );
        } else if (scanType == "tune_globalthreshold") {
            std::cout << "-> Found Global Threshold Tuning" << std::endl;
            s.reset( new Fei4GlobalThresholdTune(&bookie) );
        } else if (scanType == "tune_pixelthreshold") {
            std::cout << "-> Found Pixel Threshold Tuning" << std::endl;
            s.reset( new Fei4PixelThresholdTune(&bookie) );
        } else if (scanType == "tune_globalpreamp") {
            std::cout << "-> Found Global Preamp Tuning" << std::endl;
            s.reset( new Fei4GlobalPreampTune(&bookie) );
        } else if (scanType == "retune_globalpreamp") {
            std::cout << "-> Found Global Preamp Retuning" << std::endl;
            s.reset( new Fei4GlobalPreampRetune(&bookie) );
        } else if (scanType == "tune_pixelpreamp") {
            std::cout << "-> Found Pixel Preamp Tuning" << std::endl;
            s.reset( new Fei4PixelPreampTune(&bookie) );
        } else if (scanType == "noisescan") {
            std::cout << "-> Found Noisescan" << std::endl;
            s.reset( new Fei4NoiseScan(&bookie) );
        } else if (scanType == "selftrigger") {
            std::cout << "-> Found Selftrigger" << std::endl;
            s.reset(new Fei4Selftrigger(&bookie) );
        } else if (scanType == "selftrigger_noise") {
            std::cout << "-> Found Selftrigger" << std::endl;
            s.reset(new Fei4Selftrigger(&bookie) );
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
        std::cout << "-> Found Scan config, loading histogrammer and analysis ..." << std::endl;
        std::ifstream scanCfgFile(scanType);
        if (!scanCfgFile) {
            std::cerr << "#ERROR# Could not open scan config: " << scanType << std::endl;
            throw("buildHistogrammers failure!");
        }
        json scanCfg;
        scanCfg << scanCfgFile;
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
                    std::cout << j << std::endl;
                    if (histoCfg[std::to_string(j)]["algorithm"] == "OccupancyMap") {
                        histogrammer.addHistogrammer(new OccupancyMap());
                    } else if (histoCfg[std::to_string(j)]["algorithm"] == "TotMap") {
                        histogrammer.addHistogrammer(new TotMap());
                    } else if (histoCfg[std::to_string(j)]["algorithm"] == "Tot2Map") {
                        histogrammer.addHistogrammer(new Tot2Map());
                    } else if (histoCfg[std::to_string(j)]["algorithm"] == "L1Dist") {
                        histogrammer.addHistogrammer(new L1Dist());
                    } else if (histoCfg[std::to_string(j)]["algorithm"] == "HitsPerEvent") {
                        histogrammer.addHistogrammer(new HitsPerEvent());
                    } else {
                        std::cerr << "#ERROR# Histogrammer \"" << histoCfg[std::to_string(j)]["algorithm"] << "\" unknown, skipping!" << std::endl;
                    }
                }
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
            }
        }
    }
}


void buildAnalyses( std::map<FrontEnd*, std::unique_ptr<DataProcessor>>& analyses, const std::string& scanType, Bookkeeper& bookie, ScanBase* s, int mask_opt) {
    if (scanType.find("json") != std::string::npos) {
        std::cout << "-> Found Scan config, loading histogrammer and analysis ..." << std::endl;
        std::ifstream scanCfgFile(scanType);
        if (!scanCfgFile) {
            std::cerr << "#ERROR# Could not open scan config: " << scanType << std::endl;
            throw( "buildAnalyses failed" );
        }
        json scanCfg;
        scanCfg << scanCfgFile;
        json histoCfg = scanCfg["scan"]["histogrammer"];
        json anaCfg = scanCfg["scan"]["analysis"];

        for (FrontEnd *fe : bookie.feList ) {
            if (fe->isActive()) {
                // TODO this loads only FE-i4 specific stuff, bad
                // Load histogrammer
                // TODO hardcoded
                analyses[fe].reset( new Fei4Analysis(&bookie, dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()) );
                auto& ana = static_cast<Fei4Analysis&>( *(analyses[fe]) );
                ana.connect(s, fe->clipHisto, fe->clipResult);
                ana.addAlgorithm(new L1Analysis());
                ana.addAlgorithm(new OccupancyAnalysis());
                // Disable masking of pixels
                if(mask_opt == 0) {
                    std::cout << " -> Disabling masking for this scan!" << std::endl;
                    ana.setMasking(false);
                }
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
            }
        }
    }
}
