// #################################
// # Author: Timon Heim
// # Email: timon.heim at cern.ch
// # Project: Yarr
// # Description: Command line scan tool
// # Comment: To be used instead of gui
// ################################

#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>
#include <ctime>

#include "SpecController.h"
#include "Bookkeeper.h"
#include "Fe65p2.h"
#include "ScanBase.h"
#include "Fe65p2DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "Fei4Analysis.h"
#include "Fei4Scans.h"
#include "Fe65p2Scans.h"
#include "json.hpp"

using json=nlohmann::basic_json<std::map, std::vector, std::string, bool, std::int32_t, std::uint32_t, float>;

void printHelp();
void listScans();

#if 0
// TODO replace me with proper variable type not global
bool scanDone = false;
bool processorDone = false;

void process(Bookkeeper *bookie) {
    // Set correct Hit Discriminator setting, for proper decoding
    Fe65p2DataProcessor proc;
    proc.connect(&bookie->rawData, &bookie->eventMap);
    proc.init();
    
    while(!scanDone) {
        // TODO some better wakeup signal?
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        proc.process();
    }
    proc.process();
}

void analysis(Fei4Histogrammer *h, Fei4Analysis *a) {
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
#endif

int main(int argc, char *argv[]) {
    std::cout << "#####################################" << std::endl;
    std::cout << "# Welcome to the YARR Scan Console! #" << std::endl;
    std::cout << "#####################################" << std::endl;

    std::cout << "-> Parsing command line parameters ..." << std::endl;
    
    // Init parameters
    unsigned specNum = 0;
    std::string scanType = "";
    std::string configPath = "";
    std::string outputDir = "./";
    bool doPlots = false;

    int c;
    while ((c = getopt(argc, argv, "hs:n:c:po:")) != -1) {
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
            case 'c':
                configPath = optarg;
                break;
            case 'p':
                doPlots = true;
                break;
            case 'o':
                outputDir = std::string(optarg);
                if (outputDir.back() != '/')
                    outputDir = outputDir + "/";
                break;
            case '?':
                if (optopt == 's' || optopt == 'n' || optopt == 'c') {
                    std::cerr << "-> Option " << (char)optopt 
                        << " requires a parameter! (Proceeding with default)" << std::endl;
                } else {
                    std::cerr << "-> Unknown parameter: " << (char)optopt << std::endl;
                }
                break;
            default:
                std::cerr << "-> Error while parsing command line parameters!" << std::endl;
                return -1;
        }
    }

    std::cout << " SPEC Nr: " << specNum << std::endl;
    std::cout << " Scan Type: " << scanType << std::endl;
    std::cout << " Global configuration: " << configPath << std::endl;
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
    SpecController spec;
    spec.init(specNum);
    Bookkeeper bookie(&spec, &spec);
    bookie.initGlobalFe(new Fe65p2(&spec));
    bookie.setTargetCharge(800);
   
    // TODO move me somwhere else
    spec.setTriggerLogicMask(0x010);
    spec.setTriggerLogicMode(MODE_L1A_COUNT);

    std::cout << "-> Read global config (" << configPath << "):" << std::endl;
    std::fstream gConfig(configPath, std::ios::in);
    if (!gConfig) {
        std::cerr << "## ERROR ## Could not open file: " << configPath << std::endl;
        //return -1;
    }
    
    // TODO add fe65p2 to bookie
    /*
    while (!gConfig.eof() && gConfig) {
        unsigned id, tx, rx;
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
            gConfig >> name >> id >> tx >> rx >> feCfgPath;
            if (gConfig.eof())
                break;
            std::cout << "-> Found FE " << name << std::endl;
            // Add FE to bookkeeper
            bookie.addFe(id, tx, rx);
            bookie.getLastFe()->setName(name);
            // TODO verify cfg typea
            // Load config
            bookie.getLastFe()->fromFileBinary(feCfgPath);
            // Set chipId again after loading in case we got std cfg
            bookie.getLastFe()->setChipId(id);
            // Make backup of current config
            bookie.getLastFe()->toFileBinary(feCfgPath + "-" + std::string(timestamp));
            bookie.getLastFe()->toFileBinary(feCfgPath);
        }
    }
    */ 
    bookie.addFe(new Fe65p2(&spec), 0, 0);
    dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->setName("fe65p2");
    bookie.getLastFe()->setActive(1);
    //bookie.getLastFe()->setScap(1.18);
    //bookie.getLastFe()->setLcap(0);
    //bookie.getLastFe()->setVcalSlope(0.564);
    //bookie.getLastFe()->setVcalOffset(0.011);

    //Fe65p2 *fe = bookie.globalFe<Fe65p2>();
    //fe->setName("fe65p2");
    //fe->clipDataFei4 = &bookie.eventMap[0];
    //fe->clipHisto = &bookie.histoMap[0];
    //fe->clipResult = &bookie.resultMap[0];

    json icfg;
    std::fstream icfg_file((dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->getName()+".json").c_str(), std::ios::in);
    if (icfg_file) {
        std::fstream backup((dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->getName()+".json.backup").c_str(), std::ios::out);
        icfg_file >> icfg;
        bookie.globalFe<Fe65p2>()->fromFileJson(icfg);
        dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->fromFileJson(icfg);
        dynamic_cast<FrontEndCfg*>(bookie.getLastFe())->setName("fe65p2");
        backup << std::setw(4) << icfg;
        backup.close();
    }
    icfg_file.close();
    
    std::cout << std::endl;
    std::cout << "#################" << std::endl;
    std::cout << "# Configure FEs #" << std::endl;
    std::cout << "#################" << std::endl;
    
    std::chrono::steady_clock::time_point cfg_start = std::chrono::steady_clock::now();
    /*
    for (unsigned i=0; i<bookie.feList.size(); i++) {
        Fei4 *fe = bookie.feList[i];
        std::cout << "-> Configuring " << fe->getName() << std::endl;
        // Select correct channel
        spec.setCmdEnable(0x1 << fe->getTxChannel());
        // Configure
        fe->configure();
        fe->configurePixels(); // TODO should call abstract configure only
        // Wait for fifo to be empty
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        while(!spec.isCmdEmpty());
    }*/
    spec.setCmdEnable(0x1);
    bookie.globalFe<Fe65p2>()->configure();
    while(!spec.isCmdEmpty());

    std::chrono::steady_clock::time_point cfg_end = std::chrono::steady_clock::now();
    std::cout << "-> All FEs configured in " 
        << std::chrono::duration_cast<std::chrono::milliseconds>(cfg_end-cfg_start).count() << " ms !" << std::endl;
    
    // Wait for rx to sync with FE stream
    // TODO Check RX sync
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    // Enable all active channels
    //spec.setCmdEnable(bookie.getTxMask());
    spec.setCmdEnable(0x1);
    std::cout << "-> Setting Tx Mask to: 0x" << std::hex << bookie.getTxMask() << std::dec << std::endl;
    //spec.setRxEnable(bookie.getRxMask());
    spec.setRxEnable(0x1);
    std::cout << "-> Setting Rx Mask to: 0x" << std::hex << bookie.getRxMask() << std::dec << std::endl;
    
    std::cout << std::endl;
    std::cout << "##############" << std::endl;
    std::cout << "# Setup Scan #" << std::endl;
    std::cout << "##############" << std::endl;

    // TODO Make this nice
    ScanBase *s = NULL;
    std::cout << "-> Selecting Scan: " << scanType << std::endl;
    if (scanType == "digitalscan") {
        std::cout << "-> Found Digital Scan" << std::endl;
        s = new Fe65p2DigitalScan(&bookie);
    } else if (scanType == "analogscan") {
        std::cout << "-> Found Analog Scan" << std::endl;
        s = new Fe65p2AnalogScan(&bookie);
    } else if (scanType == "thresholdscan") {
        std::cout << "-> Found Threshold Scan" << std::endl;
        s = new Fe65p2ThresholdScan(&bookie);
    } else if (scanType == "totscan") {
        std::cout << "-> Found ToT Scan" << std::endl;
        s = new Fe65p2TotScan(&bookie);
    } else if (scanType == "tune_globalthreshold") {
        std::cout << "-> Found Global Threshold Tuning" << std::endl;
        s = new Fe65p2GlobalThresholdTune(&bookie);
    } else if (scanType == "tune_pixelthreshold") {
        std::cout << "-> Found Pixel Threshold Tuning" << std::endl;
        s = new Fe65p2PixelThresholdTune(&bookie);
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
        s = new Fe65p2NoiseScan(&bookie);
    } else if (scanType == "exttrigger") {
        std::cout << "-> Found External Trigger" << std::endl;
        s = new Fe65p2ExtTrigger(&bookie);
    } else {
        std::cout << "-> No matching Scan found, possible:" << std::endl;
        listScans();
        std::cerr << "-> Aborting!" << std::endl;
        return -1;
    }
    
    std::map<FrontEnd*, std::unique_ptr<Fei4Histogrammer> > histogrammers;
    std::map<FrontEnd*, std::unique_ptr<Fei4Analysis> >     analyses;
    
    // Init histogrammer and analysis
    for (FrontEnd *fe : bookie.feList ) {
        if (fe->isActive()) {
            // Init histogrammer per FE
            histogrammers[fe] = std::unique_ptr<Fei4Histogrammer>( new Fei4Histogrammer );
            auto& histogrammer = histogrammers[fe];
            histogrammer->connect(fe->clipData, fe->clipHisto);
            // Add generic histograms
            histogrammer->addHistogrammer(new OccupancyMap());
            histogrammer->addHistogrammer(new TotMap());
            histogrammer->addHistogrammer(new Tot2Map());
            histogrammer->addHistogrammer(new L1Dist());
            histogrammer->addHistogrammer(new HitsPerEvent());
            histogrammer->addHistogrammer(new TotDist());
            histogrammer->addHistogrammer(new DataArchiver("rawData.dat"));
            // Fe65p2 specific
            histogrammer->setMapSize(64, 64);
           
            // Init analysis per FE and depending on scan type
            analyses[fe] = std::unique_ptr<Fei4Analysis>( new Fei4Analysis(&bookie, dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()) );
            auto& ana = analyses[fe];
            ana->connect(s, fe->clipHisto, fe->clipResult);
            ana->addAlgorithm(new L1Analysis());
            ana->addAlgorithm(new TotDistPlotter());
            if (scanType == "digitalscan") {
                ana->addAlgorithm(new OccupancyAnalysis());
            } else if (scanType == "analogscan") {
                ana->addAlgorithm(new OccupancyAnalysis());
            } else if (scanType == "thresholdscan") {
                //ana->addAlgorithm(new OccupancyAnalysis());
                ana->addAlgorithm(new ScurveFitter());
            } else if (scanType == "totscan") {
	            ana->addAlgorithm(new TotAnalysis());
            } else if (scanType == "tune_globalthreshold") {
                ana->addAlgorithm(new OccGlobalThresholdTune());
            } else if (scanType == "tune_pixelthreshold") {
                ana->addAlgorithm(new OccPixelThresholdTune());
            } else if (scanType == "tune_globalpreamp") {
                ana->addAlgorithm(new TotAnalysis());
            } else if (scanType == "tune_pixelpreamp") {
                ana->addAlgorithm(new TotAnalysis());
            } else if (scanType == "noisescan") {
                ana->addAlgorithm(new NoiseAnalysis());
            } else if (scanType == "exttrigger") {
                ana->addAlgorithm(new OccupancyAnalysis());
            } else {
                std::cout << "-> Analyses not defined for scan type" << std::endl;
                listScans();
                std::cerr << "-> Aborting!" << std::endl;
                return -1;
            }
            ana->setMapSize(64, 64);
        }
    }

    std::cout << "-> Running pre scan!" << std::endl;
    s->init();
    s->preScan();

#if 0
    unsigned int numThreads = std::thread::hardware_concurrency();
    std::cout << "-> Starting " << numThreads << " processor Threads:" << std::endl; 
    std::vector<std::thread> procThreads;
    for (unsigned i=0; i<numThreads; i++) {
        procThreads.push_back(std::thread(process, &bookie));
        std::cout << "  -> Processor thread #" << i << " started!" << std::endl;
    }
#endif

    std::vector<std::thread> anaThreads;
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

    Fe65p2DataProcessor proc;
    proc.connect( &bookie.rawData, &bookie.eventMap );
    proc.init();
    proc.run();

    std::cout << std::endl;
    std::cout << "########" << std::endl;
    std::cout << "# Scan #" << std::endl;
    std::cout << "########" << std::endl;

    std::cout << "-> Starting scan!" << std::endl;
    std::chrono::steady_clock::time_point scan_start = std::chrono::steady_clock::now();
    s->run();
    s->postScan();
    std::cout << "-> Scan done!" << std::endl;
    
    proc.scanDone = true;
    bookie.rawData.cv.notify_all();
    
    std::chrono::steady_clock::time_point scan_done = std::chrono::steady_clock::now();
    std::cout << "-> Waiting for processors to finish ..." << std::endl;
    proc.join();
    
    std::chrono::steady_clock::time_point processor_done = std::chrono::steady_clock::now();
    Fei4Histogrammer::processorDone = true;
    Fei4Analysis::histogrammerDone = true;
    for( auto& histogrammer : histogrammers ) {
      histogrammer.second->join();
    }
    for( auto& ana : analyses ) {
      ana.second->join();
    }
    std::cout << "-> Processor done, waiting for analysis ..." << std::endl;
    std::chrono::steady_clock::time_point all_done = std::chrono::steady_clock::now();
    std::cout << "-> All done!" << std::endl;

    spec.setCmdEnable(0x0);
    spec.setRxEnable(0x0);

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
    for ( FrontEnd* fe : bookie.feList ) {
        if (fe->isActive()) {
            // Save config
            std::cout << "-> Saving config of FE " << dynamic_cast<FrontEndCfg*>(fe)->getName() << std::endl;
            json cfg;
            std::fstream cfg_file((dynamic_cast<FrontEndCfg*>(fe)->getName()+".json").c_str(), std::ios::out);
            dynamic_cast<FrontEndCfg*>(fe)->toFileJson(cfg);
            cfg_file << std::setw(4) << cfg;
            cfg_file.close();
            // Plot
            if (doPlots) {
                std::cout << "-> Plotting histograms of FE " << dynamic_cast<FrontEndCfg*>(fe)->getRxChannel() << std::endl;
                auto &output = *fe->clipResult;

                std::string name = /*std::string(timestamp) + "-" + */dynamic_cast<FrontEndCfg*>(fe)->getName() + "_ch" + std::to_string(dynamic_cast<FrontEndCfg*>(fe)->getRxChannel()) + "_" + scanType;

                while(!output.empty()) {
                    std::unique_ptr<HistogramBase> histo = output.popData();
                    histo->plot(name, outputDir);
                    histo->toFile(name, outputDir);
                }
            }
        }
    }
    return 0;
}

void printHelp() {
    std::cout << "Help:" << std::endl;
    std::cout << " -h: Shows this." << std::endl;
    std::cout << " -s <scan_type> : Scan type. Possible types:" << std::endl;
    listScans();
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


