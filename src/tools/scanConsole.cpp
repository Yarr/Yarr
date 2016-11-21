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
#include <vector>
#include <iomanip>
#include <cctype> //w'space detection
#include <ctime>
#include <map>
#include <sstream>

#include "SpecController.h"
#include "TxCore.h"
#include "RxCore.h"
#include "Bookkeeper.h"
#include "Fei4.h"
#include "ScanBase.h"
#include "Fei4DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "Fei4Analysis.h"
#include "Fei4Scans.h"

#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)

//  #include <errno.h>
//  #include <sys/stat.h>
#include <cstdlib> //I am not proud of this ):

#endif

void printHelp();
void listScans();

// TODO replace me with proper variable type not global
bool scanDone = false;
bool processorDone = false;

void process(Bookkeeper *bookie) {
    // Set correct Hit Discriminator setting, for proper decoding
    Fei4DataProcessor proc(bookie->g_fe->getValue(&Fei4::HitDiscCnfg));
    proc.connect(&bookie->rawData, &bookie->eventMap);
    proc.init();
    
    while(!scanDone) {
        // TODO some better wakeup signal?
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        proc.process();
    }
    proc.process();
}

void analysis(Fei4Histogrammer *h, Fei4Analysis *a){
    h->init();
    a->init();
    
    while(!processorDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        h->process();
        a->process();
    }
    h->process();
    a->process();

    a->end();
}

int main(int argc, char *argv[]) {
    std::cout << "#####################################" << std::endl;
    std::cout << "# Welcome to the YARR Scan Console! #" << std::endl;
    std::cout << "#####################################" << std::endl;

    std::cout << "-> Parsing command line parameters ..." << std::endl;
    
    // Init parameters
    unsigned specNum = 0;
    std::string scanType = "";
    std::string gConfigPath = "";
    std::vector<std::string> cConfigPath;
    std::string outputDir = "../Plots/";
    bool doPlots = false;
    unsigned runCounter = 0;
    std::string runDir = "run";

#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)
    {
        std::ifstream iF("../.yarr-system/.runCounter");
        iF >> runCounter;
        iF.close();
        std::stringstream sS;
        sS << std::setw(7) << std::setfill('0') << runCounter;
        runDir += sS.str();
        runDir += '/';

        runCounter += 1;
        std::ofstream oF("../.yarr-system/.runCounter");
        oF << runCounter;
        oF.close();
    }
#endif

    int c;
    while ((c = getopt(argc, argv, "hs:n:g:c:po:")) != -1) {
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
            case 'g':
                gConfigPath = std::string(optarg);
                break;
            case 'c':
//                configPath = std::string(optarg);
                optind -= 1; //this is a bit hacky, but getopt doesn't support multiple
                             //values for one option, so it can't be helped
                for(; optind < argc && *argv[optind] != '-'; optind += 1){
                    cConfigPath.push_back(std::string(argv[optind]));
                }
                break;
            case 'p':
                doPlots = true;
                break;
            case 'o':
                outputDir = std::string(optarg);
                if (outputDir.back() != '/')
                    outputDir = outputDir + "/";
#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)
                outputDir += runDir;
#endif
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

#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)

//for some reason, 'make' issues that kdir is an undefined reference
//a test program on another machine has worked fine
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

#endif

    if(gConfigPath != ""){
        std::cout << std::endl << "##############################################" << std::endl
                               << "##                                          ##" << std::endl
                               << "##  Extracting configs from config list...  ##" << std::endl
                               << "##                                          ##" << std::endl
                               << "##############################################" << std::endl;
        int tmpI = 0;
        std::string tmpStr = "";
        std::ifstream gCfgIn(gConfigPath);
        if(!gCfgIn){
            std::cerr << "Cannot open file containing config list" << std::endl;
        }else{
            while(std::getline(gCfgIn, tmpStr)){
                if(tmpStr.front() == '#'){
                    continue;
                }
                while(isspace(tmpStr.back())){
                    tmpStr.pop_back();
                }
                cConfigPath.push_back(tmpStr);
            }
        }
        if(cConfigPath.size() == 0){
            std::cerr << "No chips added. Terminating program..." << std::endl;
            return -1;
        }
    }

    std::cout << " SPEC Nr: " << specNum << std::endl;
    std::cout << " Scan Type: " << scanType << std::endl;
    if(gConfigPath != ""){
        std::cout << " Config list in: " << gConfigPath << std::endl;
    }
    std::cout << "Chips: " << std::endl;
    for(std::string const& sTmp : cConfigPath){
        std::cout << "    " << sTmp << std::endl;
    }
    std::cout << " Output Plots: " << doPlots << std::endl;
    std::cout << " Output Directory: " << outputDir << std::endl;

    // Timestamp
    std::time_t now = std::time(NULL);
    struct tm *lt = std::localtime(&now);
    char timestamp[20];
    strftime(timestamp, 20, "%F_%H:%M:%S", lt);
    std::cout << std::endl;
    std::cout << "Timestamp : " << timestamp << std::endl;

    std::cout << std::endl;
    std::cout << "#################" << std::endl;
    std::cout << "# Init Hardware #" << std::endl;
    std::cout << "#################" << std::endl;


    std::cout << "-> Init SPEC " << specNum << " : " << std::endl;
    SpecController spec(specNum);
    TxCore tx(&spec);
    RxCore rx(&spec);
    Bookkeeper bookie(&tx, &rx);
    std::map<FrontEnd*, std::string> feCfgMap;
    bookie.setTargetThreshold(2500);

    std::cout << "#######################" << std::endl
              << "##                   ##" << std::endl
              << "##  Adding chips...  ##" << std::endl
              << "##                   ##" << std::endl
              << "#######################" << std::endl;
    std::cout << cConfigPath.size() << " chips..." << std::endl; //DEBUG
    std::cout << "Token 1" << std::endl;
    for(std::string const& sTmp : cConfigPath){
        std::string discardMe; //Error handling, wait for user
        nlohmann::json jTmp;
        std::ifstream iFTmp(sTmp);
        try{
            if(iFTmp){
                jTmp = nlohmann::json::parse(iFTmp);
            }else{
                throw std::invalid_argument("Config file not readable... ");
            }
        }catch(std::invalid_argument){
            std::cerr << "No config in " << sTmp
                      << ", press enter to skip, enter '4' to add an empty FE-I4B config, "
                      << "enter '6' to add an empty FE65-P2 config: " << std::endl;
            std::getline(std::cin, discardMe);
            if(discardMe == "4"){
                jTmp["FE-I4B"] = nlohmann::json::parse("{}");
            }else if(discardMe == "6"){
                jTmp["FE65-P2"] = nlohmann::json::parse("{}");
            }else{
                continue;
            }
        }
        std::cout << "Token 2" << std::endl;
        iFTmp.close();
        std::string chipType;
        if(!jTmp["FE-I4B"].is_null()){
            chipType = "FE-I4B";
        }else if(!jTmp["FE65-P2"].is_null()){
            chipType = "FE65-P2";
        }else{
            std::cerr << "Unknown chip type or malformed config in " << sTmp << std::endl;
            continue;
        }
        std::cout << "Token 3" << std::endl;

        unsigned int uTmpTx = 0;
        unsigned int uTmpRx = 0;

        //determine TxChannel
        std::cout << chipType << std::endl;
        if(jTmp[chipType]["Parameter"]["TxChannel"].is_null()){
            std::cout << "Token 4" << std::endl;
            uTmpTx = 0;
            while(bookie.isChannelUsed(uTmpTx)){
                uTmpTx += 1;
            }
            std::cout << "No TxChannel in " << sTmp << ", "<< uTmpTx << " can be used"
                      << ". Press enter to confirm or enter another TxChannel:  " << std::endl;
            std::getline(std::cin, discardMe);
            if(discardMe.size() > 0){
                uTmpTx = std::stoul(discardMe);
            }
        }else{
            if(bookie.isChannelUsed(jTmp[chipType]["Parameter"]["TxChannel"])){
                uTmpTx = 0;
                while(bookie.isChannelUsed(uTmpTx)){
                    uTmpTx += 1;
                }
                std::cout << "TxChannel " << jTmp[chipType]["Parameter"]["TxChannel"]
                          << " from config " << sTmp << "already in use, " << uTmpTx
                          << " can be used. Press enter to confirm or enter another TxChannel: " << std::endl;
                std::getline(std::cin, discardMe);
                if(discardMe.size() > 0){
                    uTmpTx = std::stoul(discardMe);
                }
            }else{
                uTmpTx = jTmp[chipType]["Parameter"]["TxChannel"];
            }
        }
        std::cout << "Token 5" << std::endl;
        //determine RxChannel
        if(jTmp[chipType]["Parameter"]["TxChannel"].is_null()){
            if(!jTmp[chipType]["Parameter"]["RxChannel"].is_null()){
                uTmpRx = jTmp[chipType]["Parameter"]["RxChannel"];
                std::cout << "Warning! RxChannel " << uTmpRx
                          << " found in config, but using automatically generated TxChannel "
                          << uTmpTx << ". Press enter to confirm or enter another RxChannel: " << std::endl;
                std::getline(std::cin, discardMe);
                if(discardMe.size() > 0){
                    uTmpRx = std::stoul(discardMe);
                }
            }else{
                std::cout << "Token 6" << std::endl;
                uTmpRx = uTmpTx;
                std::cout << "Warning! Neither TxChannel nor RxChannel in config " << sTmp
                          << ". Automatically generated RxChannel " << uTmpRx
                          << " can be used. Press enter to confirm or enter another RxChannel: " << std::endl;
                std::getline(std::cin, discardMe);
                if(discardMe.size() > 0){
                    uTmpRx = std::stoul(discardMe);
                }
            }
        }else{
            if(jTmp[chipType]["Parameter"]["TxChannel"] == uTmpTx){
                if(!jTmp[chipType]["Parameter"]["RxChannel"].is_null()){
                    uTmpRx = jTmp[chipType]["Parameter"]["RxChannel"];
                }else{
                    uTmpRx = uTmpTx;
                    std::cout << "Warning! No RxChannel in config " << sTmp
                              << ". Using " << uTmpRx << " from TxChannel instead. "
                              << "Press enter to confirm or enter another RxChannel: " << std::endl;
                    std::getline(std::cin, discardMe);
                    if(discardMe.size() > 0){
                        uTmpRx = std::stoul(discardMe);
                    }
                }
            }else{
                if(jTmp[chipType]["Parameter"]["RxChannel"].is_null()){
                    uTmpRx = uTmpTx;
                    std::cout << "Warning! No RxChannel in " << sTmp
                              << ". Using automatically generated RxChannel " << uTmpRx
                              << ". Press enter to confirm or enter another RxChannel: " << std::endl;
                    std::getline(std::cin, discardMe);
                    if(discardMe.size() > 0){
                        uTmpRx = std::stoul(discardMe);
                    }
                }else{
                    uTmpRx = jTmp[chipType]["Parameter"]["RxChannel"];
                    std::cout << "Warning! Found RxChannel " << uTmpRx << " in config " << sTmp
                              << ", but using automatically generated TxChannel " << uTmpTx
                              << ". Press enter to confirm or enter another RxChannel: " << std::endl;
                    std::getline(std::cin, discardMe);
                    if(discardMe.size() > 0){
                        uTmpRx = std::stoul(discardMe);
                    }
                }
            }
        }
        std::cout << "Token 7" << std::endl;

        if(bookie.isChannelUsed(uTmpTx)){
            std::cerr << "ERROR! Cannot add " << chipType << " with config " << sTmp
                      << ", TxChannel " << uTmpTx << ", RxChannel " << uTmpRx << std::endl;
            continue;
        }

        if(chipType == "FE-I4B"){
            Fei4 *fEI4Tmp = new Fei4(&tx, uTmpTx, uTmpRx);
            try{
                fEI4Tmp->fromFileJson(jTmp);
            }catch(std::domain_error){
                std::cerr << "Malformed config. Press enter to proceed, "
                          << "enter 's' to skip this chip or CTRL+C to abort... " <<std::endl;
                std::getline(std::cin, discardMe);
                if(discardMe == "s"){
                    std::cout << "Skipping config file " << sTmp << ", not adding chip" << std::endl;
                    continue;
                }
            }
            bookie.addFe(fEI4Tmp, uTmpTx, uTmpRx);
            feCfgMap.insert(std::pair<FrontEnd*, std::string>(fEI4Tmp, sTmp));
        }else{
            std::cout << "Token 8" << std::endl;
            Fe65p2 *fE65P2Tmp = new Fe65p2(&tx, uTmpTx, uTmpRx);
            std::cout << "Token 8.1" << std::endl;
            try{
                fE65P2Tmp->fromFileJson(jTmp);
            }catch(std::domain_error){
                std::cerr << "Malformed config. Press enter to proceed, "
                          << "enter 's' to skip this chip or CTRL+C to abort... " <<std::endl;
                std::getline(std::cin, discardMe);
                if(discardMe == "s"){
                    std::cout << "Skipping config file " << sTmp << ", not adding chip" << std::endl;
                    continue;
                }
            }
            std::cout << "Token 8.2" << std::endl;
            bookie.addFe(fE65P2Tmp, uTmpTx, uTmpRx);
            feCfgMap.insert(std::pair<FrontEnd*, std::string>(fE65P2Tmp, sTmp));
            std::cout << "Token 9" << std::endl;
        }
    }
    std::cout << "Token 10" << std::endl;

/*    std::cout << "-> Read global config (" << configPath << "):" << std::endl;
    std::fstream gConfig(configPath, std::ios::in);
    if (!gConfig) {
        std::cerr << "## ERROR ## Could not open file: " << configPath << std::endl;
        return -1;
    }

    while (!gConfig.eof() && gConfig) {
        unsigned id, txChannel, rxChannel;
        std::string name, feCfgPath;
        char peekaboo = gConfig.peek();
        if (peekaboo == '\n') {
            gConfig.ignore();
            peekaboo = gConfig.peek();
        }
        if (peekaboo == '#') {
            char tmp[1024];
            gConfig.getline(tmp, 1024);
            std::cout << " Skipping: " << tmp << std::endl;
        } else {
            gConfig >> name >> id >> txChannel >> rxChannel >> feCfgPath;
            if (gConfig.eof())
                break;
            std::cout << "-> Found FE " << name << std::endl;
            // Add FE to bookkeeper
            bookie.addFe(new Fei4(&tx, txChannel, rxChannel), txChannel, rxChannel);
            dynamic_cast<FrontEnd*>(bookie.getLastFe())->setName(name);
            // TODO verify cfg typea
            // Load config
            dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->fromFileBinary(feCfgPath);
            // Set chipId again after loading in case we got std cfg
            //dynamic_cast<FrontEnd*>(bookie.getLastFe())->setChipId(id);
            // Make backup of current config
            dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->toFileBinary(feCfgPath + "-" + std::string(timestamp));
            dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->toFileBinary(feCfgPath);
        }
    }
*/
    std::cout << std::endl;
    std::cout << "#################" << std::endl;
    std::cout << "# Configure FEs #" << std::endl;
    std::cout << "#################" << std::endl;
    
    std::chrono::steady_clock::time_point cfg_start = std::chrono::steady_clock::now();
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        std::cout << "-> Configuring " << fe->getName() << std::endl;
        // Select correct channel
        tx.setCmdEnable(0x1 << fe->getTxChannel());
        // Configure
        fe->configure();
        // Wait for fifo to be empty
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        while(!tx.isCmdEmpty());
    }
    std::chrono::steady_clock::time_point cfg_end = std::chrono::steady_clock::now();
    std::cout << "-> All FEs configured in " 
        << std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count() << " ms !" << std::endl;
    
    // Wait for rx to sync with FE stream
    // TODO Check RX sync
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    // Enable all active channels
    tx.setCmdEnable(bookie.getTxMask());
    std::cout << "-> Setting Tx Mask to: 0x" << std::hex << bookie.getTxMask() << std::dec << std::endl;
    rx.setRxEnable(bookie.getRxMask());
    std::cout << "-> Setting Rx Mask to: 0x" << std::hex << bookie.getRxMask() << std::dec << std::endl;
    
    std::cout << std::endl;
    std::cout << "##############" << std::endl;
    std::cout << "# Setup Scan #" << std::endl;
    std::cout << "##############" << std::endl;

    // TODO Make this nice
    ScanBase *s = NULL;

    if (scanType.find("json") != std::string::npos) {
        std::cout << "-> Found Scan config, constructing scan ..." << std::endl;

    } else {
        std::cout << "-> Selecting Scan: " << scanType << std::endl;
        if (scanType == "digitalscan") {
            std::cout << "-> Found Digital Scan" << std::endl;
            s = new Fei4DigitalScan(&bookie);
        } else if (scanType == "analogscan") {
            std::cout << "-> Found Analog Scan" << std::endl;
            s = new Fei4AnalogScan(&bookie);
        } else if (scanType == "thresholdscan") {
            std::cout << "-> Found Threshold Scan" << std::endl;
            s = new Fei4ThresholdScan(&bookie);
        } else if (scanType == "totscan") {
            std::cout << "-> Found ToT Scan" << std::endl;
            s = new Fei4TotScan(&bookie);
        } else if (scanType == "tune_globalthreshold") {
            std::cout << "-> Found Global Threshold Tuning" << std::endl;
            s = new Fei4GlobalThresholdTune(&bookie);
        } else if (scanType == "tune_pixelthreshold") {
            std::cout << "-> Found Pixel Threshold Tuning" << std::endl;
            s = new Fei4PixelThresholdTune(&bookie);
        } else if (scanType == "tune_globalpreamp") {
            std::cout << "-> Found Global Preamp Tuning" << std::endl;
            s = new Fei4GlobalPreampTune(&bookie);
        } else if (scanType == "retune_globalpreamp") {
            std::cout << "-> Found Global Preamp Retuning" << std::endl;
            s = new Fei4GlobalPreampRetune(&bookie);
        } else if (scanType == "tune_pixelpreamp") {
            std::cout << "-> Found Pixel Preamp Tuning" << std::endl;
            s = new Fei4PixelPreampTune(&bookie);
        } else if (scanType == "noisescan") {
            std::cout << "-> Found Noisescan" << std::endl;
            s = new Fei4NoiseScan(&bookie);
        } else {
            std::cout << "-> No matching Scan found, possible:" << std::endl;
            listScans();
            std::cerr << "-> Aborting!" << std::endl;
            return -1;
        }
    }
    
    // Init histogrammer and analysis
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        if (fe->isActive()) {
            // Init histogrammer per FE
            fe->histogrammer = new Fei4Histogrammer();
            fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
            // Add generic histograms
            fe->histogrammer->addHistogrammer(new OccupancyMap());
            fe->histogrammer->addHistogrammer(new TotMap());
            fe->histogrammer->addHistogrammer(new Tot2Map());
            fe->histogrammer->addHistogrammer(new L1Dist());
            fe->histogrammer->addHistogrammer(new HitsPerEvent());
           
            // Init analysis per FE and depending on scan type
            fe->ana = new Fei4Analysis(&bookie, fe->getRxChannel());
            fe->ana->connect(s, fe->clipHisto, fe->clipResult);
            fe->ana->addAlgorithm(new L1Analysis());
            if (scanType == "digitalscan") {
                fe->ana->addAlgorithm(new OccupancyAnalysis());
            } else if (scanType == "analogscan") {
                fe->ana->addAlgorithm(new OccupancyAnalysis());
            } else if (scanType == "thresholdscan") {
                fe->ana->addAlgorithm(new ScurveFitter());
            } else if (scanType == "totscan") {
                fe->ana->addAlgorithm(new TotAnalysis());
            } else if (scanType == "tune_globalthreshold") {
                fe->ana->addAlgorithm(new OccGlobalThresholdTune());
            } else if (scanType == "tune_pixelthreshold") {
                fe->ana->addAlgorithm(new OccPixelThresholdTune());
            } else if (scanType == "tune_globalpreamp") {
                fe->ana->addAlgorithm(new TotAnalysis());
            } else if (scanType == "tune_pixelpreamp") {
                fe->ana->addAlgorithm(new TotAnalysis());
            } else if (scanType == "noisescan") {
                fe->ana->addAlgorithm(new NoiseAnalysis());
            } else {
                std::cout << "-> Analyses not defined for scan type" << std::endl;
                listScans();
                std::cerr << "-> Aborting!" << std::endl;
                return -1;
            }
        }
    }

    std::cout << "-> Running pre scan!" << std::endl;
    s->init();
    std::cout << "Token 11" << std::endl;
    s->preScan();
    std::cout << "Token 12" << std::endl;

    unsigned int numThreads = std::thread::hardware_concurrency();
    std::cout << "-> Starting " << numThreads << " processor Threads:" << std::endl; 
    std::vector<std::thread> procThreads;
    for (unsigned i=0; i<numThreads; i++) {
        procThreads.push_back(std::thread(process, &bookie));
        std::cout << "  -> Processor thread #" << i << " started!" << std::endl;
    }

    std::vector<std::thread> anaThreads;
    std::cout << "-> Starting histogrammer and analysis threads:" << std::endl;
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        if (fe->isActive()) {
            anaThreads.push_back(std::thread(analysis, fe->histogrammer, fe->ana));
            std::cout << "  -> Analysis thread of Fe " << fe->getRxChannel() << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "########" << std::endl;
    std::cout << "# Scan #" << std::endl;
    std::cout << "########" << std::endl;

    std::cout << "-> Starting scan!" << std::endl;
    std::chrono::steady_clock::time_point scan_start = std::chrono::steady_clock::now();
    s->run();
    s->postScan();
    std::cout << "-> Scan done!" << std::endl;
    scanDone = true;
    std::chrono::steady_clock::time_point scan_done = std::chrono::steady_clock::now();
    std::cout << "-> Waiting for processors to finish ..." << std::endl;
    for (unsigned i=0; i<numThreads; i++) {
        procThreads[i].join();
    }
    std::chrono::steady_clock::time_point processor_done = std::chrono::steady_clock::now();
    processorDone = true;
    std::cout << "-> Processor done, waiting for analysis ..." << std::endl;
    for (unsigned i=0; i<anaThreads.size(); i++) {
        anaThreads[i].join();
    }
    std::chrono::steady_clock::time_point all_done = std::chrono::steady_clock::now();
    std::cout << "-> All done!" << std::endl;

    tx.setCmdEnable(0x0);
    rx.setRxEnable(0x0);

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
    
    // Cleanup
    delete s;
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        FrontEnd *fe = bookie.feList[i];
        if (fe->isActive()) {
            // Save config
            std::cout << "-> Saving config of FE " << fe->getName() << std::endl;
            //dynamic_cast<FrontEndCfg*>(fe)->toFileBinary();
            nlohmann::json jTmp;
            dynamic_cast<FrontEndCfg*>(fe)->toFileJson(jTmp);
            std::string jSTmp;
            jSTmp = jTmp.dump(4); //4 spaces indentation
            std::stringstream sStrmTmp;
            sStrmTmp << jSTmp;
            std::ofstream oFTmp(feCfgMap.at(fe));
            oFTmp << sStrmTmp.rdbuf();
            oFTmp.close();
            // Plot
            if (doPlots) {
                std::cout << "-> Plotting histograms of FE " << fe->getRxChannel() << std::endl;
#if defined(__linux__) || defined(__APPLE__) && defined(__MACH__)
                std::string outputDirTmp = outputDir + runDir;
                outputDirTmp += fe->getName() + "/" + scanType + "/";
                std::string cmdStr = "mkdir -p ";
                cmdStr += outputDirTmp;
                int sysExSt = system(cmdStr.c_str());
                fe->ana->plot(std::string(timestamp) + "-" + fe->getName() + "_ch" + std::to_string(fe->getRxChannel()) + "_" + scanType, outputDirTmp);
#else
                fe->ana->plot(std::string(timestamp) + "-" + fe->getName() + "_ch" + std::to_string(fe->getRxChannel()) + "_" + scanType, outputDir);
#endif
                //fe->ana->toFile(std::string(timestamp) + "-" + fe->getName() + "_ch" + std::to_string(fe->getRxChannel()) + "_" + scanType, outputDir);
            }
            // Free
            delete fe->histogrammer;
            fe->histogrammer = NULL;
            delete fe->ana;
            fe->ana = NULL;
        }
    }
    return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -s <scan_type> : Scan type. Possible types:" << std::endl;
    listScans();
    std::cout << " -n: Provide SPECboard number." << std::endl;
    std::cout << " -g <cfg_list.txt>: Provide list of chip configurations." << std::endl;
    std::cout << " -c <cfg1.cfg> [<cfg2.cfg> ...]: Provide chip configuration, can take multiple arguments." << std::endl;
    std::cout << " -p: Enable plotting of results." << std::endl;
    std::cout << " -o <dir> : Output directory." << std::endl;
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
}


