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
#include "RunManager.h"

/*void analysis(unsigned ch, ScanBase *s, ClipBoard<Fei4Data> *events) {
    ClipBoard<HistogramBase> clipHisto;
    ClipBoard<HistogramBase> clipResult;

    std::cout << "### Histogramming data ###" << std::endl;
    Fei4Histogrammer histogrammer;
    histogrammer.addHistogrammer(new OccupancyMap());
    //histogrammer.addHistogrammer(new TotMap());
    //histogrammer.addHistogrammer(new Tot2Map());
    //histogrammer.addHistogrammer(new L1Dist());
    histogrammer.connect(events, &clipHisto);
    histogrammer.process();
    
    std::cout << "Collected: " << clipHisto.size() << " Histograms on Channel " << ch << std::endl;

    std::cout << "### Analyzing data ###" << std::endl;
    Fei4Analysis ana;
    ana.addAlgorithm(new OccupancyAnalysis);
    ana.connect(s, &clipHisto, &clipResult);
    ana.init();
    ana.process();
    ana.end();
    std::cout << "Collected: " << clipResult.size() << " Histograms on Channel " << ch << std::endl;

    std::cout << "### Saving ###" << std::endl;
    std::string channel = "ch" + std::to_string(ch);
    ana.plot(channel+ "_analogscan");
    //histogrammer.toFile("analogscan");
    std::cout << "... done!" << std::endl;
}*/

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
	keeper.data = &clipRaw;

	keeper.addFe(0,0);
	keeper.addFe(1,0);
	keeper.g_fe->setChipId(8);


		// Booking of histrograms and corresponding analysis
	for(unsigned int k=0; k<keeper.feList.size(); k++) {
		keeper.feList[k]->histogrammer = new Fei4Histogrammer();
		keeper.feList[k]->ana = new Fei4Analysis();
		keeper.feList[k]->histogrammer->addHistogrammer(new OccupancyMap());
		keeper.feList[k]->ana->addAlgorithm(new OccupancyAnalysis);
	}

	RunManager rm(&keeper);
	keeper.prepareMap();




    Fei4AnalogScan anaScan(&keeper);

    std::chrono::steady_clock::time_point init = std::chrono::steady_clock::now();
    std::cout << "### Init Scan ###" << std::endl;
    anaScan.init();

    std::cout << "### Configure Module ###" << std::endl;

 	for(unsigned int k=0; k<keeper.feList.size(); k++) {
		tx.setCmdEnable(0x1);
		keeper.feList[k]->setRunMode(false);
		keeper.feList[k]->configure();
		keeper.feList[k]->configurePixels();
		while(!tx.isCmdEmpty());
	}   

	tx.setCmdEnable(0x3);
    rx.setRxEnable(0xF);


    std::this_thread::sleep_for(std::chrono::microseconds(1000));

    std::chrono::steady_clock::time_point config = std::chrono::steady_clock::now();
    std::cout << "### Pre Scan ###" << std::endl;
    anaScan.preScan();

    std::cout << "### Scan ###" << std::endl;
    anaScan.run();

    std::cout << "### Post Scan ###" << std::endl;
    anaScan.postScan();

    std::cout << "### Disabling RX ###" << std::endl;
    tx.setCmdEnable(0x0);
    rx.setRxEnable(0x0);
    std::cout << "Collected: " << clipRaw.size() << " Raw Data Fragments" << std::endl;
    std::chrono::steady_clock::time_point scan = std::chrono::steady_clock::now();

    std::cout << "### Processing data ###" << std::endl;
    Fei4DataProcessor proc(keeper.g_fe->getValue(&Fei4::HitDiscCnfg));
    proc.connect(&clipRaw, keeper.eventMap);
    proc.init();
    proc.process();
    std::cout << "Collected: " << keeper.eventMap[0]->size() << " Events" << std::endl;
    std::chrono::steady_clock::time_point pro = std::chrono::steady_clock::now();

	rm.threadEvaluation(&anaScan);

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
    return 0;
}
