
#include <iostream>
#include <stdint.h>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

#include "SpecController.h"
#include "TxCore.h"
#include "Fei4.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Fei4EventData.h"
#include "Fei4DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "HistogramBase.h"
#include "Fei4Analysis.h"

#include "Fei4Scans.h"

bool scanDone = false;
bool processorDone = false;

void processing(Fei4 *fe, ClipBoard<RawDataContainer> *clipRaw, std::map<unsigned, ClipBoard<Fei4Data> > *eventMap) {
    std::cout << "### Processing data ###" << std::endl;
    Fei4DataProcessor proc(fe->getValue(&Fei4::HitDiscCnfg));
    proc.connect(clipRaw, eventMap);
    proc.init();
    while(!scanDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        proc.process();
    }
    proc.process();

}

void analysis(unsigned ch, ScanBase *s, ClipBoard<Fei4Data> *events) {
    ClipBoard<HistogramBase> clipHisto;
    ClipBoard<HistogramBase> clipResult;

    std::cout << "### Histogramming data on channel #" << ch << " ###" << std::endl;
    Fei4Histogrammer histogrammer;
    histogrammer.addHistogrammer(new OccupancyMap());
    histogrammer.connect(events, &clipHisto);
 
    std::cout << "### Analyzing data on channel #" << ch << " ###" << std::endl;
    Fei4Analysis ana;
	ana.channel = ch;

    ana.addAlgorithm(new OccPixelThresholdTune,ch);
    ana.connect(s, &clipHisto, &clipResult);
    ana.init();
    


    while(!processorDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        histogrammer.process();
        ana.process();
    }
    histogrammer.process();

    ana.process();

    ana.end();
    std::cout << "Collected: " << clipResult.size() << " Histograms on Channel " << ch << std::endl;

    std::cout << "### Saving on channel #" << ch << " ###" << std::endl;
    std::string channel = "ch" + std::to_string(ch);
    ana.plot(channel+ "_thresholdpixeltune");
    ana.toFile(channel + "_thresholdpixeltune");
    std::cout << "... done!" << std::endl;
}

int main(void) {
    // Start Stopwatch
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    // Init
    std::cout << "### Init Stuff ###" << std::endl;
    SpecController spec(0);
    TxCore tx(&spec);
    RxCore rx(&spec);

    ClipBoard<RawDataContainer> clipRaw;
    Bookkeeper keeper(&tx, &rx);
	keeper.rawData = &clipRaw;

    std::string cfgName = "test_config.bin";

	keeper.addFe(0,0);
	keeper.addFe(1,0);

	keeper.g_fe->setChipId(8);
    keeper.g_fe->fromFileBinary(cfgName);

		// Booking of histrograms and corresponding analysis
	for(unsigned int k=0; k<keeper.activeFeList.size(); k++) {
	    keeper.feList[k]->fromFileBinary(cfgName);
//		keeper.feList[k]->histogrammer = new Fei4Histogrammer();
//		keeper.feList[k]->ana = new Fei4Analysis();
//		keeper.feList[k]->histogrammer->addHistogrammer(new OccupancyMap());
//		keeper.feList[k]->ana->addAlgorithm(new OccPixelThresholdTune);
	}

	Fei4PixelThresholdTune thrTune(&keeper);

    std::chrono::steady_clock::time_point init = std::chrono::steady_clock::now();
    std::cout << "### Init Scan ###" << std::endl;
    thrTune.init();

    std::cout << "### Configure Module ###" << std::endl;

	for(unsigned int k=0; k<keeper.activeFeList.size(); k++) {
		tx.setCmdEnable(1 << keeper.activeFeList[k]->getChannel());
		keeper.feList[k]->setRunMode(false);
		keeper.feList[k]->configure();
		keeper.feList[k]->configurePixels();
		while(!tx.isCmdEmpty());
	}

	tx.setCmdEnable(0x3);
    rx.setRxEnable(0x3);
    
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    
    std::chrono::steady_clock::time_point config = std::chrono::steady_clock::now();
    std::cout << "### Pre Scan ###" << std::endl;
    thrTune.preScan();

    std::thread p1(processing, keeper.g_fe, &clipRaw, &keeper.eventMap);
	std::thread p2(processing, keeper.g_fe, &clipRaw, &keeper.eventMap);
	std::thread p3(processing, keeper.g_fe, &clipRaw, &keeper.eventMap);
	std::thread p4(processing, keeper.g_fe, &clipRaw, &keeper.eventMap);

    
		// Only one thread per active FE!
    std::thread t1(analysis, 0, &thrTune, &keeper.eventMap[0]);
    std::thread t2(analysis, 1, &thrTune, &keeper.eventMap[1]);
    
    std::cout << "### Scan ###" << std::endl;
    thrTune.run();

    std::cout << "### Post Scan ###" << std::endl;
    thrTune.postScan();

    std::cout << "### Disabling RX ###" << std::endl;
    tx.setCmdEnable(0x0);
    rx.setRxEnable(0x0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    scanDone = true;
    
    std::cout << "Collected: " << clipRaw.size() << " Raw Data Fragments" << std::endl;
    std::chrono::steady_clock::time_point scan = std::chrono::steady_clock::now();

    p1.join();
    p2.join();
    p3.join();
    p4.join();

    processorDone = true;
    
 	for(unsigned int k=0; k<keeper.activeFeList.size(); k++) {
	    std::cout << "Collected on channel #" << keeper.activeFeList[k]->getChannel() << ": " << keeper.eventMap[0].size() << " Events" << std::endl;
	}
    std::chrono::steady_clock::time_point pro = std::chrono::steady_clock::now();
    
		// Only one thread per active FE!
    t1.join();
    t2.join();
    /*t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    t9.join();
    t10.join();
    t11.join();
    t12.join();
    t13.join();
    t14.join();
    t15.join();
    t16.join();*/

    std::chrono::steady_clock::time_point ana = std::chrono::steady_clock::now();

    std::cout << "### Timing ###" << std::endl;
    std::cout << "Init: " << std::chrono::duration_cast<std::chrono::milliseconds>(init-start).count() << " ms!" << std::endl;
    std::cout << "Configure: " << std::chrono::duration_cast<std::chrono::milliseconds>(config - init).count() << " ms!" << std::endl;
    std::cout << "Scan: " << std::chrono::duration_cast<std::chrono::milliseconds>(scan - config).count() << " ms!" << std::endl;
    std::cout << "Processing: " << std::chrono::duration_cast<std::chrono::milliseconds>(pro - scan).count() << " ms!" << std::endl;
    std::cout << "Analysis: " << std::chrono::duration_cast<std::chrono::milliseconds>(ana - pro).count() << " ms!" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << "Total: " << std::chrono::duration_cast<std::chrono::milliseconds>(ana - init).count() << " ms!" << std::endl;
    std::cout << std::endl << "Finished!" << std::endl;

    std::cout << "Saving config to: " << cfgName << std::endl;
    keeper.g_fe->toFileBinary(cfgName);
    return 0;
}
