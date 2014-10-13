#include <iostream>
#include <stdint.h>
#include <fstream>
#include <string>

#include "SpecController.h"
#include "TxCore.h"
#include "Fei4.h"
#include "ClipBoard.h"
#include "RawData.h"
#include "Fei4EventData.h"
#include "Fei4DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "ResultBase.h"

#include "Fei4Scans.h"

int main(void) {
    // Init
    std::cout << "### Init Stuff ###" << std::endl;
    SpecController spec(0);
    TxCore tx(&spec);
    RxCore rx(&spec);

    Fei4 g_fe(&tx, 0);
    Fei4 fe(&tx, 0);

    ClipBoard<RawData> clipRaw;
    ClipBoard<Fei4Data> clipEvent;
    ClipBoard<ResultBase> clipHisto;

    Fei4AnalogScan anaScan(&g_fe, &tx, &rx, &clipRaw);
    
    std::cout << "### Init Scan ###" << std::endl;
    anaScan.init();

    std::cout << "### Configure Module ###" << std::endl;
    tx.setCmdEnable(0x1);
    fe.setRunMode(false);
    fe.configure();
    fe.configurePixels();
    while(!tx.isCmdEmpty());
    rx.setRxEnable(0x1);

    std::cout << "### Pre Scan ###" << std::endl;
    anaScan.preScan();

    std::cout << "### Scan ###" << std::endl;
    anaScan.run();

    std::cout << "### Post Scan ###" << std::endl;
    anaScan.postScan();
    
    std::cout << "### Disabling RX ###" << std::endl;
    tx.setCmdEnable(0x0);
    rx.setRxEnable(0x0);
    
    std::cout << "### Analyzing data ###" << std::endl;
    Fei4DataProcessor proc(fe.getValue(&Fei4::HitDiscCnfg));
    proc.connect(&clipRaw, &clipEvent);
    proc.process();
    
    std::cout << "### Histogramming data ###" << std::endl;
    Fei4Histogrammer histogrammer;
    histogrammer.addHistogrammer(new OccupancyMap());
    histogrammer.addHistogrammer(new TotMap());
    histogrammer.addHistogrammer(new Tot2Map());
    histogrammer.addHistogrammer(new L1Dist());
    histogrammer.connect(&clipEvent, &clipHisto);
    histogrammer.process();
    histogrammer.publish();

    std::cout << "### Saving ###" << std::endl;
    histogrammer.plot("analogscan");
    histogrammer.toFile("analogscan");
    std::cout << "... done!" << std::endl;

    return 0;
}
