#include <iostream>
#include <stdint.h>
#include <fstream>
#include <string>
#include <chrono>

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
    ClipBoard<Fei4Data> clipEvent;
    ClipBoard<HistogramBase> clipHisto;
    ClipBoard<HistogramBase> clipResult;

    Fei4ThresholdScan anaScan(&g_fe, &tx, &rx, &clipRaw);
    
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
    proc.connect(&clipRaw, &clipEvent);
    proc.process();
    std::cout << "Collected: " << clipEvent.size() << " Events" << std::endl;
    std::chrono::steady_clock::time_point pro = std::chrono::steady_clock::now();
    
    
    std::cout << "### Histogramming data ###" << std::endl;
    Fei4Histogrammer histogrammer;
    histogrammer.addHistogrammer(new OccupancyMap());
    //histogrammer.addHistogrammer(new TotMap());
    //histogrammer.addHistogrammer(new Tot2Map());
    //histogrammer.addHistogrammer(new L1Dist());
    histogrammer.connect(&clipEvent, &clipHisto);
    histogrammer.process();
    std::chrono::steady_clock::time_point hist = std::chrono::steady_clock::now();

    std::cout << "Collected: " << clipHisto.size() << " Histograms" << std::endl;

    std::cout << "### Analyzing data ###" << std::endl;
    Fei4Analysis ana;
    ana.addAlgorithm(new OccupancyAnalysis);
    ana.addAlgorithm(new ScurveFitter);
    ana.connect(&anaScan, &clipHisto, &clipResult);
    ana.init();
    ana.process();
    ana.end();
    std::cout << "Collected: " << clipResult.size() << " Histograms" << std::endl;
    std::chrono::steady_clock::time_point anal = std::chrono::steady_clock::now();

    std::cout << "### Saving ###" << std::endl;
    ana.plot("analogscan");
    //histogrammer.toFile("analogscan");
    std::cout << "... done!" << std::endl;

    std::cout << "### Timing ###" << std::endl;
    std::cout << "Init: " << std::chrono::duration_cast<std::chrono::milliseconds>(init-start).count() << " ms!" << std::endl;
    std::cout << "Configure: " << std::chrono::duration_cast<std::chrono::milliseconds>(config - init).count() << " ms!" << std::endl;
    std::cout << "Scan: " << std::chrono::duration_cast<std::chrono::milliseconds>(scan - config).count() << " ms!" << std::endl;
    std::cout << "Processor: " << std::chrono::duration_cast<std::chrono::milliseconds>(pro - scan).count() << " ms!" << std::endl;
    std::cout << "Histogrammer: " << std::chrono::duration_cast<std::chrono::milliseconds>(hist - pro).count() << " ms!" << std::endl;
    std::cout << "Analysis: " << std::chrono::duration_cast<std::chrono::milliseconds>(anal - hist).count() << " ms!" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << "Total: " << std::chrono::duration_cast<std::chrono::milliseconds>(anal - init).count() << " ms!" << std::endl;
    std::cout << std::endl << "Finished!" << std::endl;
    return 0;
}
