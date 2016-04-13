#ifndef YARRGUI_SCANS_CPP
#define YARRGUI_SCANS_CPP

void YarrGui::setCustomScan(CustomScan & other) {
    cs = other;
    //if(!cs.isScanEmpty()) {
        ui->runCustomScanButton->setEnabled(true);
    //}

    return;
}

void YarrGui::doScan(QString qn) {
    int N = ui->feTree->topLevelItemCount();
    int M = scanVec.size();
    for(int j = 0; j < N; j++) {
        scanDone = false;
        processorDone = false;

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; } //Is the Scan checkbox checked?

        Fei4 * fe = bk->feList.at(j);

        ScanBase * s = nullptr;

        if(qn == "NS")   {s = new Fei4NoiseScan(bk);}
        if(qn == "DS")   {s = new Fei4DigitalScan(bk);}
        if(qn == "AS")   {s = new Fei4AnalogScan(bk);}
        if(qn == "TS")   {s = new Fei4ThresholdScan(bk);}
        if(qn == "ToTS") {s = new Fei4TotScan(bk);}
        if(qn == "GTT")  {s = new Fei4GlobalThresholdTune(bk);}
        if(qn == "GPT")  {s = new Fei4GlobalPreampTune(bk);}
        if(qn == "PTT")  {s = new Fei4PixelThresholdTune(bk);}
        if(qn == "PPT")  {s = new Fei4PixelPreampTune(bk);}
        if(qn == "CS")   {s = &cs;}

        if(s == nullptr) {
            std::cerr << "Invalid scan QString parameter passed or scan object construction failed. Returning...\n";
            return;
        }

        QTreeWidgetItem * plotTreeItem = nullptr; //Plot tree item for current FE
        for(int k = 0; k < ui->plotTree->topLevelItemCount(); k++) { //Is the current FE already in the tree? ...
           if(ui->plotTree->topLevelItem(k)->text(0) == ui->feTree->topLevelItem(j)->text(0)) {
               plotTreeItem = ui->plotTree->topLevelItem(k);
               break;
           }
        }

        if(plotTreeItem == nullptr) { //... if not: create a branch for it
            plotTreeItem = new QTreeWidgetItem(ui->plotTree);
            plotTreeItem->setText(0, ui->feTree->topLevelItem(j)->text(0));
        }

        QTreeWidgetItem * plotTreeItemDS = new QTreeWidgetItem(); //plot tree item for current scan
        plotTreeItemDS->setText(0, qn);
        plotTreeItem->addChild(plotTreeItemDS);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);

        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);

        if (qn=="CS") {
            CustomScan * tmp;
            tmp = dynamic_cast<CustomScan*>(s);
            if(tmp->bA.at(OCC_MAP) == true) {fe->histogrammer->addHistogrammer(new OccupancyMap());}
            if(tmp->bA.at(TOT_MAP) == true) {fe->histogrammer->addHistogrammer(new TotMap());}
            if(tmp->bA.at(TOT_2_MAP) == true) {fe->histogrammer->addHistogrammer(new Tot2Map());}

            if(tmp->bA.at(OCC_ANA) == true) {fe->ana->addAlgorithm(new OccupancyAnalysis());}
            if(tmp->bA.at(NOISE_ANA) == true) {fe->ana->addAlgorithm(new NoiseAnalysis());}
            if(tmp->bA.at(TOT_ANA) == true) {fe->ana->addAlgorithm(new TotAnalysis());}
            if(tmp->bA.at(S_CU_FIT) == true) {fe->ana->addAlgorithm(new ScurveFitter());}
            if(tmp->bA.at(PIX_THR) == true) {fe->ana->addAlgorithm(new OccPixelThresholdTune);}
        } else {
            fe->histogrammer->addHistogrammer(new OccupancyMap());

            if (qn == "ToTS" || qn == "GPT" || qn == "PPT") {
                fe->histogrammer->addHistogrammer(new TotMap());
                fe->histogrammer->addHistogrammer(new Tot2Map());
            }

            if(qn == "NS")   {fe->ana->addAlgorithm(new NoiseAnalysis());}
            if(qn == "DS")   {fe->ana->addAlgorithm(new OccupancyAnalysis());}
            if(qn == "AS")   {fe->ana->addAlgorithm(new OccupancyAnalysis());}
            if(qn == "TS")   {fe->ana->addAlgorithm(new ScurveFitter());}
            if(qn == "ToTS") {fe->ana->addAlgorithm(new TotAnalysis());}
            if(qn == "GTT")  {fe->ana->addAlgorithm(new OccGlobalThresholdTune());}
            if(qn == "GPT")  {fe->ana->addAlgorithm(new TotAnalysis());}
            if(qn == "PTT")  {fe->ana->addAlgorithm(new OccPixelThresholdTune());}
            if(qn == "PPT")  {fe->ana->addAlgorithm(new TotAnalysis());}
        }

        s->init();
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //1
        s->preScan();
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //2

        unsigned int numThreads = std::thread::hardware_concurrency();
        //std::cout << "-> Starting " << numThreads << " processor Threads:" << std::endl;
        std::vector<std::thread> procThreads;
        for (unsigned i=0; i<numThreads; i++) {
            procThreads.push_back(std::thread(process, bk, &scanDone));
            //std::cout << "  -> Processor thread #" << i << " started!" << std::endl;
        }

        std::vector<std::thread> anaThreads;
        //std::cout << "-> Starting histogrammer and analysis threads:" << std::endl;
        if (fe->isActive()) {
            anaThreads.push_back(std::thread(analysis, fe->histogrammer, fe->ana, &processorDone));
            //std::cout << "  -> Analysis thread of Fe " << fe->getRxChannel() << std::endl;
        }

        s->run();
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //3
        s->postScan();
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //4
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //5

        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //6

        if(qn != "CS") {
            delete s;
        }

//        fe->toFileBinary();
//        fe->ana->plot("Scan_GUI");

        //DEBUG begin [plotting]

        //clear raw data
        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        //clear raw data
        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0) {
            HistogramBase * showMe = fe->clipResult->popData();

            //Add tab to plot tab widget
            QCustomPlot * tabScanPlot = new QCustomPlot(ui->scanPlots_tabWidget); // X
            QWidget * addToTabWidget = dynamic_cast<QWidget*>(tabScanPlot);
            QString newTabName = qn + ' ' + ui->feTree->topLevelItem(j)->text(0);
            ui->scanPlots_tabWidget->addTab(addToTabWidget, newTabName);

            //Add plot to scan tree
            QTreeWidgetItem * plotTreeItemP = new QTreeWidgetItem(plotTreeItemDS); // X
            plotTreeItemP->setText(0, "Plot " + QString::number(plotTreeItemDS->childCount())
                                              + " (" + QString::fromStdString(showMe->getName()) + ")");
            plotTreeItemDS->addChild(plotTreeItemP);

            if(dynamic_cast<Histo2d*>(showMe) != nullptr) {
                Histo2d * myHist2d = dynamic_cast<Histo2d*>(showMe);
                //Create plot

                QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
                QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);

                tabScanPlot->addPlottable(colorMap);
                tabScanPlot->plotLayout()->insertRow(0);
                tabScanPlot->plotLayout()->addElement(0, 0, new QCPPlotTitle(tabScanPlot, QString::fromStdString(myHist2d->getName())));

                colorMap->setName(QString::fromStdString(myHist2d->getName()));
                colorMap->data()->setSize(80, 336);
                colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
                colorMap->keyAxis()->setLabel(QString::fromStdString(myHist2d->getXaxisTitle()));
                colorMap->valueAxis()->setLabel(QString::fromStdString(myHist2d->getYaxisTitle()));
                for(int xCoord = 0; xCoord<80; xCoord++) {
                    for(int yCoord = 0; yCoord<336; yCoord++) {
                        double colVal = myHist2d->getBin(yCoord + 336*xCoord);              //TODO make better
                        colorMap->data()->setCell(xCoord, yCoord, colVal);
                    }
                }

                tabScanPlot->plotLayout()->addElement(1, 1, colorScale);
                colorScale->setType(QCPAxis::atRight);
                colorMap->setColorScale(colorScale);
                colorScale->axis()->setLabel(QString::fromStdString(myHist2d->getZaxisTitle()));
                colorMap->setGradient(QCPColorGradient::gpPolar);
                colorMap->rescaleDataRange();

            } else if(dynamic_cast<Histo1d*>(showMe) != nullptr) {
                Histo1d * myHist1d = dynamic_cast<Histo1d*>(showMe);

                QCPBars * myBars = new QCPBars(tabScanPlot->xAxis, tabScanPlot->yAxis);
                tabScanPlot->addPlottable(myBars);
                myBars->setName(QString::fromStdString(myHist1d->getName()));

                for(unsigned int i = 0; i < myHist1d->size(); i++) {
                    myBars->addData((double)(i+1), myHist1d->getBin(i));
                }

                myBars->rescaleAxes();
                tabScanPlot->plotLayout()->insertRow(0);
                tabScanPlot->plotLayout()->addElement(0, 0, new QCPPlotTitle(tabScanPlot, QString::fromStdString(myHist1d->getName())));
                myBars->keyAxis()->setLabel(QString::fromStdString(myHist1d->getXaxisTitle()));
                myBars->valueAxis()->setLabel(QString::fromStdString(myHist1d->getYaxisTitle()));

            } else {
                std::cerr << "Correct plot type not found or severe cast error\n";                  //DEBUG
                return;
            }

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();
        }
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //7
        delete fe->histogrammer;
        fe->histogrammer = nullptr;
        delete fe->ana;
        fe->ana = nullptr;
    }
    return;
}

void YarrGui::on_doScansButton_clicked() {
    ui->scanProgressBar->setValue(0);
    unsigned int M = scanVec.size();
    for(unsigned int i = 0; i < M; i++) {
        doScan(scanVec.at(i));
        ui->scanProgressBar->setValue(100*(i+1)/M);
    }
    ui->scanProgressBar->setValue(100);
}

void YarrGui::on_RemoveScans_Button_clicked() {
    ui->scanVec_lineEdit->setText("");
    scanVec.clear();
}

void YarrGui::on_runCustomScanButton_clicked() {
    doScan("CS");
    ui->runCustomScanButton->setEnabled(false);

    return;
}

#endif
