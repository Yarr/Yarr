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

void analysis(unsigned ch, ScanBase *s, ClipBoard<Fei4Data> *events) {
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
}

int main(void) {
    // Start Stopwatch
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    // Init
    std::cout << "### Init Stuff ###" << std::endl;
    SpecController spec(0);
    TxCore tx(&spec);
    RxCore rx(&spec);

    Fei4 g_fe(&tx, 0);
    Fei4 fe(&tx, 0);

    ClipBoard<RawData> clipRaw;
    std::map<unsigned, ClipBoard<Fei4Data>* > eventMap;
    ClipBoard<Fei4Data> clipEvent0;
    ClipBoard<Fei4Data> clipEvent1;
    ClipBoard<Fei4Data> clipEvent2;
    ClipBoard<Fei4Data> clipEvent3;
    ClipBoard<Fei4Data> clipEvent4;
    ClipBoard<Fei4Data> clipEvent5;
    ClipBoard<Fei4Data> clipEvent6;
    ClipBoard<Fei4Data> clipEvent7;

    eventMap[0] = &clipEvent0;
    eventMap[1] = &clipEvent1;
    eventMap[2] = &clipEvent2;
    eventMap[3] = &clipEvent3;
    eventMap[4] = &clipEvent4;
    eventMap[5] = &clipEvent5;
    eventMap[6] = &clipEvent6;
    eventMap[7] = &clipEvent7;

    Fei4AnalogScan anaScan(&g_fe, &tx, &rx, &clipRaw);

    std::chrono::steady_clock::time_point init = std::chrono::steady_clock::now();
    std::cout << "### Init Scan ###" << std::endl;
    anaScan.init();

    std::cout << "### Configure Module ###" << std::endl;
    tx.setCmdEnable(0x1);
    fe.setRunMode(false);
    fe.configure();
    fe.configurePixels();
    while(!tx.isCmdEmpty());
    rx.setRxEnable(0x1);

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
    Fei4DataProcessor proc(fe.getValue(&Fei4::HitDiscCnfg));
    proc.connect(&clipRaw, eventMap);
    proc.init();
    proc.process();
    std::cout << "Collected: " << clipEvent1.size() << " Events" << std::endl;
    std::chrono::steady_clock::time_point pro = std::chrono::steady_clock::now();

    std::thread t1(analysis, 0, &anaScan, &clipEvent0);
    std::thread t2(analysis, 1, &anaScan, &clipEvent1);
    std::thread t3(analysis, 2, &anaScan, &clipEvent2);
    std::thread t4(analysis, 3, &anaScan, &clipEvent3);
    std::thread t5(analysis, 4, &anaScan, &clipEvent4);
    std::thread t6(analysis, 5, &anaScan, &clipEvent5);
    std::thread t7(analysis, 6, &anaScan, &clipEvent6);
    std::thread t8(analysis, 7, &anaScan, &clipEvent7);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();

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
