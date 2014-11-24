
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

void processing(Fei4 *fe, ClipBoard<RawData> *clipRaw, std::map<unsigned, ClipBoard<Fei4Data>* > eventMap) {
    std::cout << "### Processing data ###" << std::endl;
    Fei4DataProcessor proc(fe->getValue(&Fei4::HitDiscCnfg));
    proc.connect(clipRaw, eventMap);
    proc.init();
    while(!scanDone) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        proc.process();
    }
    proc.process();
    processorDone = true;
}

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
    
    std::cout << "Collected: " << clipHisto.size() << " Histograms on Channel " << ch << std::endl;

    std::cout << "### Analyzing data ###" << std::endl;
    Fei4Analysis ana;
    //ana.addAlgorithm(new OccupancyAnalysis);
    //ana.addAlgorithm(new ScurveFitter);
    ana.addAlgorithm(new OccGlobalThresholdTune);
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

    std::cout << "### Saving ###" << std::endl;
    std::string channel = "ch" + std::to_string(ch);
    ana.plot(channel+ "_thresholdtune");
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
    ClipBoard<Fei4Data> clipEvent8;
    ClipBoard<Fei4Data> clipEvent9;
    ClipBoard<Fei4Data> clipEvent10;
    ClipBoard<Fei4Data> clipEvent11;
    ClipBoard<Fei4Data> clipEvent12;
    ClipBoard<Fei4Data> clipEvent13;
    ClipBoard<Fei4Data> clipEvent14;
    ClipBoard<Fei4Data> clipEvent15;

    eventMap[0] = &clipEvent0;
    /*eventMap[1] = &clipEvent1;
    eventMap[2] = &clipEvent2;
    eventMap[3] = &clipEvent3;
    eventMap[4] = &clipEvent4;
    eventMap[5] = &clipEvent5;
    eventMap[6] = &clipEvent6;
    eventMap[7] = &clipEvent7;
    eventMap[8] = &clipEvent8;
    eventMap[9] = &clipEvent9;
    eventMap[10] = &clipEvent10;
    eventMap[11] = &clipEvent11;
    eventMap[12] = &clipEvent12;
    eventMap[13] = &clipEvent13;
    eventMap[14] = &clipEvent14;
    eventMap[15] = &clipEvent15;*/

    Fei4GlobalThresholdTune thrTune(&g_fe, &tx, &rx, &clipRaw);

    std::chrono::steady_clock::time_point init = std::chrono::steady_clock::now();
    std::cout << "### Init Scan ###" << std::endl;
    thrTune.init();

    std::cout << "### Configure Module ###" << std::endl;
    tx.setCmdEnable(0x1);
    fe.setRunMode(false);
    fe.configure();
    fe.configurePixels();
    while(!tx.isCmdEmpty());
    rx.setRxEnable(0x1);
    
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    
    std::chrono::steady_clock::time_point config = std::chrono::steady_clock::now();
    std::cout << "### Pre Scan ###" << std::endl;
    thrTune.preScan();

    std::thread p1(processing, &g_fe, &clipRaw, eventMap);
    //std::thread p2(processing, &g_fe, &clipRaw, eventMap);
    //std::thread p3(processing, &g_fe, &clipRaw, eventMap);
    //std::thread p4(processing, &g_fe, &clipRaw, eventMap);
    
    std::thread t1(analysis, 0, &thrTune, &clipEvent0);
    /*std::thread t2(analysis, 1, &thrTune, &clipEvent1);
    std::thread t3(analysis, 2, &thrTune, &clipEvent2);
    std::thread t4(analysis, 3, &thrTune, &clipEvent3);
    std::thread t5(analysis, 4, &thrTune, &clipEvent4);
    std::thread t6(analysis, 5, &thrTune, &clipEvent5);
    std::thread t7(analysis, 6, &thrTune, &clipEvent6);
    std::thread t8(analysis, 7, &thrTune, &clipEvent7);
    std::thread t9(analysis, 8, &thrTune, &clipEvent8);
    std::thread t10(analysis, 9, &thrTune, &clipEvent9);
    std::thread t11(analysis, 10, &thrTune, &clipEvent10);
    std::thread t12(analysis, 11, &thrTune, &clipEvent11);
    std::thread t13(analysis, 12, &thrTune, &clipEvent12);
    std::thread t14(analysis, 13, &thrTune, &clipEvent13);
    std::thread t15(analysis, 14, &thrTune, &clipEvent14);
    std::thread t16(analysis, 15, &thrTune, &clipEvent15);*/
    
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
    //p2.join();
    //p3.join();
    //p4.join();
    
    std::cout << "Collected: " << clipEvent0.size() << " Events" << std::endl;
    std::chrono::steady_clock::time_point pro = std::chrono::steady_clock::now();
    

    t1.join();
    /*t2.join();
    t3.join();
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
    return 0;
}
