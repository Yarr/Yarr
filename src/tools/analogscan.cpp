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

void analysis(unsigned ch, ScanBase *s, Bookkeeper *k) {
    std::cout << "### Histogramming data ###" << std::endl;
	Fei4 *fe = k->getFei4byChannel(ch);
    fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
    fe->histogrammer->process();
    
	std::cout << "Collected: " << fe->clipHisto->size() << " Histograms on Channel " << ch << " with ChipID " << fe->getChipId() << std::endl;

    std::cout << "### Analyzing data on channel #" << ch << " ###" << std::endl;
    fe->ana->connect(s, fe->clipHisto, fe->clipResult);
    fe->ana->init();
    fe->ana->process();
    fe->ana->end();
    std::cout << "Collected: " << fe->clipResult->size() << " Histograms on Channel " << ch << std::endl;

    std::cout << "### Saving on channel #" << ch << " ###" << std::endl;
    std::string channel = "ch" + std::to_string(ch);
    fe->ana->plot(channel + "_analogscan_");// + "_" + typeid(s).name());
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


    Bookkeeper keeper(&tx, &rx);

	keeper.addFe(0,0);
	keeper.addFe(0,1);
	keeper.g_fe->setChipId(8);


		// Booking of histrograms and corresponding analysis
	for(unsigned int k=0; k<keeper.feList.size(); k++) {
		keeper.feList[k]->histogrammer = new Fei4Histogrammer();
		keeper.feList[k]->ana = new Fei4Analysis();
		keeper.feList[k]->histogrammer->addHistogrammer(new OccupancyMap());
		keeper.feList[k]->ana->addAlgorithm(new OccupancyAnalysis);
	}

    Fei4AnalogScan anaScan(&keeper);

    std::chrono::steady_clock::time_point init = std::chrono::steady_clock::now();
    std::cout << "### Init Scan ###" << std::endl;
    anaScan.init();

    std::cout << "### Configure Module ###" << std::endl;

 	for(unsigned int k=0; k<keeper.feList.size(); k++) {
		tx.setCmdEnable(1 << keeper.feList[k]->getChannel());	// That makes more sense, right?
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
    std::cout << "Collected: " << keeper.rawData.size() << " Raw Data Fragments" << std::endl;
    std::chrono::steady_clock::time_point scan = std::chrono::steady_clock::now();

    std::cout << "### Processing data ###" << std::endl;
    Fei4DataProcessor proc(keeper.g_fe->getValue(&Fei4::HitDiscCnfg));
    proc.connect(&keeper.rawData, &keeper.eventMap);
    proc.init();
    proc.process();
 	for(unsigned int k=0; k<keeper.feList.size(); k++) {
	    std::cout << "Collected on channel #" << keeper.feList[k]->getChannel() << ": " << keeper.eventMap[0].size() << " Events" << std::endl;
	}
    std::chrono::steady_clock::time_point pro = std::chrono::steady_clock::now();

		// Only one thread per active FE!
    std::thread t1(analysis, 0, &anaScan, &keeper);
    std::thread t2(analysis, 1, &anaScan, &keeper);
/*    std::thread t3(analysis, 2, &anaScan, &keeper);
    std::thread t4(analysis, 3, &anaScan, &keeper);
    std::thread t5(analysis, 4, &anaScan, &keeper);
    std::thread t6(analysis, 5, &anaScan, &keeper);
    std::thread t7(analysis, 6, &anaScan, &keeper);
    std::thread t8(analysis, 7, &anaScan, &keeper);
    std::thread t9(analysis, 8, &anaScan, &keeper);
    std::thread t10(analysis, 9, &anaScan, &keeper);
    std::thread t11(analysis, 10, &anaScan, &keeper);
    std::thread t12(analysis, 11, &anaScan, &keeper);
    std::thread t13(analysis, 12, &anaScan, &keeper);
    std::thread t14(analysis, 13, &anaScan, &keeper);
    std::thread t15(analysis, 14, &anaScan, &keeper);
    std::thread t16(analysis, 15, &anaScan, &keeper);

*/

    t1.join();
    t2.join();
/*    t3.join();
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
    t16.join();

*/


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
