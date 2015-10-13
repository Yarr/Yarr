#include "yarrgui.h"
#include "ui_yarrgui.h"

#include <BitFile.h>
#include <BenchmarkTools.h>

#include <iostream>
#include <fstream>
#include <string>

#include <QMessageBox>

YarrGui::YarrGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::YarrGui)
{
    ui->setupUi(this);

    this->init();

    // Get list of devices
    devicePath.setPath("/dev/");
    QStringList filter;
    filter << "spec*";
    devicePath.setFilter(QDir::System);
    deviceList = devicePath.entryList(filter);
    
    // Display devices in list
    ui->device_comboBox->addItems(deviceList);

    // Init device vector
    for(int i=0; i<deviceList.size(); i++) {
        specVec.push_back(new SpecController());
    }

    // Init Benchmark Plot
    ui->benchmark_plot->xAxis->setLabel("Package size [kB]");
    ui->benchmark_plot->yAxis->setLabel("Speed [MB/s]");
    ui->benchmark_plot->xAxis->setRange(0, 500);
    ui->benchmark_plot->yAxis->setRange(0, 500);
    ui->benchmark_plot->setInteraction(QCP::iRangeDrag, true);
    ui->benchmark_plot->setInteraction(QCP::iRangeZoom, true);
    ui->benchmark_plot->setInteraction(QCP::iSelectPlottables, true);
    ui->benchmark_plot->setInteraction(QCP::iSelectAxes, true);
    ui->benchmark_plot->legend->setVisible(true);
    ui->benchmark_plot->legend->setFont(QFont("Helvetica", 9));
    ui->benchmark_plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);

//    ui->scanPlot->xAxis->setLabel("Column");
//    ui->scanPlot->yAxis->setLabel("Row");
//    ui->scanPlot->xAxis->setRange(0, 80);
//    ui->scanPlot->yAxis->setRange(0, 336);
//    ui->scanPlot->setInteraction(QCP::iRangeDrag, true);
//    ui->scanPlot->setInteraction(QCP::iRangeZoom, true);
//    ui->scanPlot->setInteraction(QCP::iSelectPlottables, true);
//    ui->scanPlot->setInteraction(QCP::iSelectAxes, true);
//    ui->scanPlot->legend->setVisible(false);
//    ui->scanPlot->legend->setFont(QFont("Helvetica", 9));
//    ui->scanPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);

    ui->global_cfg_checkBox->setChecked(true); //DEBUG
    ui->configfileName_2->setText("util/global_config.cfg"); //DEBUG

    QPen pen;
    QColor color;
    for(int i=0; i<deviceList.size(); i++) {
        writeGraphVec.push_back(ui->benchmark_plot->addGraph()); 
        readGraphVec.push_back(ui->benchmark_plot->addGraph());
        
        QColor color1(sin(i*0.3)*100+100, sin(i*0.6+0.7)*100+100, sin(i*0.4+0.6)*100+100);
        pen.setColor(color1);
        writeGraphVec[i]->setPen(pen);
        writeGraphVec[i]->setName("Spec #" + QString::number(i) + " DMA Write");
        writeGraphVec[i]->setLineStyle(QCPGraph::lsLine);
        writeGraphVec[i]->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color1, 5));
 
        QColor color2(sin(i*0.6+0.7)*100+100, sin(i*0.3)*100+100, sin(i*0.4+0.6)*100+100);
        pen.setColor(color2);
        readGraphVec[i]->setPen(pen);
        readGraphVec[i]->setName("Spec #" + QString::number(i) + " DMA Read");
        readGraphVec[i]->setLineStyle(QCPGraph::lsLine);;
        readGraphVec[i]->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color2, 5));
    }

    // Init console
    qout = new QDebugStream(std::cout, ui->console, QColor("black"));
    qerr = new QDebugStream(std::cerr, ui->console, QColor("red"));

    ui->main_tabWidget->setCurrentIndex(0);
    ui->main_tabWidget->setTabEnabled(0, true);
    ui->main_tabWidget->setTabEnabled(1, false);
    ui->main_tabWidget->setTabEnabled(2, false);
    ui->main_tabWidget->setTabEnabled(3, false);
    ui->main_tabWidget->setTabEnabled(4, false);

    ui->feTree->setColumnWidth(0, 200);
    ui->feTree->setColumnWidth(1, 500);
    //ui->feTree->setColumnWidth(2, 2000);

    ui->scanVec_lineEdit->setReadOnly(true);
}

YarrGui::~YarrGui()
{
    // Clean up devices
    for(int i=0; i<deviceList.size(); i++) {
        if (specVec[i]->isInitialized())
            delete specVec[i];
    }

    delete ui;
}

void YarrGui::on_device_comboBox_currentIndexChanged(int index)
{
    if (specVec.size()) {
        ui->specid_value->setNum(specVec[index]->getId());
        ui->bar0_value->setNum(specVec[index]->getBarSize(0));
        ui->bar4_value->setNum(specVec[index]->getBarSize(4));
    }
}


void YarrGui::init() {
    // Preset labels
    ui->specid_value->setNum(-1);
    ui->bar0_value->setNum(-1);
    ui->bar4_value->setNum(-1);
}

void YarrGui::on_init_button_clicked()
{
    int index = ui->device_comboBox->currentIndex();
    if (specVec.size() == 0 || index > specVec.size()) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Device not found!");
    } else {
        specVec[index]->init(index);
        if (specVec[index]->isInitialized()) {
            ui->specid_value->setNum(specVec[index]->getId());
            ui->bar0_value->setNum(specVec[index]->getBarSize(0));
            ui->bar4_value->setNum(specVec[index]->getBarSize(4));
            ui->main_tabWidget->setTabEnabled(1, true);
            ui->main_tabWidget->setTabEnabled(2, true);
            ui->main_tabWidget->setTabEnabled(3, true);
            ui->main_tabWidget->setTabEnabled(4, true);
            tx = new TxCore(specVec[index]);
            rx = new RxCore(specVec[index]);
            bk = new Bookkeeper(tx, rx);
        } else {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Initialization not successful!");
        }
    }
}

void YarrGui::on_progfile_button_clicked() {
    QString filename = QFileDialog::getOpenFileName(this, tr("Select bit-file"), "./", tr("Bit File(*.bit)"));
    std::fstream file(filename.toStdString().c_str(), std::fstream::in);
    if (!file && BitFile::checkFile(file)) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Selected bit file looks bad!");
        ui->progfile_name->setText("");
        return;
    }
    ui->progfile_name->setText(filename);
}

void YarrGui::on_prog_button_clicked() {
    
    int index = ui->device_comboBox->currentIndex();
    if (specVec.size() == 0 || index > specVec.size()) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Device not found!");
    } else {
        if (!specVec[index]->isInitialized()) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Spec not initiliazed!");
            return;
        }

        std::fstream file(ui->progfile_name->text().toStdString().c_str(), std::fstream::in);
        if (!file) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Problem opening File!");
            return;
        }

        size_t size = BitFile::getSize(file);
        
        // Read file into buffer
        char *buffer = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        
        // Program FPGA
        int wrote = specVec[index]->progFpga(buffer, size);
        if (wrote != size) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "FPGA not succesfully programmed!");
        }
        delete buffer;
        file.close();    
    }
}

void YarrGui::on_minSize_spinBox_valueChanged(int i) {
    ui->maxSize_spinBox->setMinimum(i+1);
}

void YarrGui::on_maxSize_spinBox_valueChanged(int i) {
    ui->minSize_spinBox->setMaximum(i-1);
}

void YarrGui::on_startWrite_button_clicked() {
    unsigned min = ui->minSize_spinBox->value();
    unsigned max = ui->maxSize_spinBox->value();
    unsigned steps = ui->steps_spinBox->value();
    unsigned repetitions = ui->repetitions_spinBox->value();

    unsigned interval = (max-min)/steps;

    for (unsigned int index=0; index<deviceList.size(); index++) {
        writeGraphVec[index]->clearData();
        
        if (specVec[index]->isInitialized()) {
            for (unsigned i=min; i<=max; i+=interval) {
                double speed = BenchmarkTools::measureWriteSpeed(specVec[index], i*256, repetitions);
                if (speed < 0) {
                    QMessageBox errorBox;
                    errorBox.critical(0, "Error", "DMA timed out!");
                    return;
                }
                writeGraphVec[index]->addData(i, speed);
                ui->benchmark_plot->rescaleAxes();
                ui->benchmark_plot->replot();
                double per = ((i-min)/(double)interval)/(double)steps;
                ui->benchmark_progressBar->setValue(per*100);
                QApplication::processEvents(); // Else we look like we are not responding
            }
        }
    }       
}

void YarrGui::on_startRead_button_clicked() {
    unsigned min = ui->minSize_spinBox->value();
    unsigned max = ui->maxSize_spinBox->value();
    unsigned steps = ui->steps_spinBox->value();
    unsigned repetitions = ui->repetitions_spinBox->value();

    unsigned interval = (max-min)/steps;

    for (unsigned int index=0; index<deviceList.size(); index++) {
        readGraphVec[index]->clearData();
        
        if (specVec[index]->isInitialized()) {
            for (unsigned i=min; i<=max; i+=interval) {
                double speed = BenchmarkTools::measureReadSpeed(specVec[index], i*256, repetitions);
                if (speed < 0) {
                    QMessageBox errorBox;
                    errorBox.critical(0, "Error", "DMA timed out!");
                    return;
                }
                readGraphVec[index]->addData(i, speed);
                ui->benchmark_plot->rescaleAxes();
                ui->benchmark_plot->replot();
                double per = ((i-min)/(double)interval)/(double)steps;
                ui->benchmark_progressBar->setValue(per*100);
                QApplication::processEvents(); // Else we look like we are not responding
            }
        }
    }       
}

void YarrGui::on_sbefile_button_2_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select EEPROM content file"), "./", tr("SpecBoard EEPROM content file(*.sbe)"));
    std::fstream file(filename.toStdString().c_str(), std::fstream::in);
    if (!file) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "A problem occured. ");
        ui->sbefile_name->setText("");
        return;
    }
    ui->sbefile_name->setText(filename);
}

void YarrGui::on_SBEWriteButton_clicked()
{
    int index = ui->device_comboBox->currentIndex();
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    std::string pathname;
    if (!(ui->SBECheckBox->isChecked())) {
        pathname = ui->sbefile_name->text().toStdString().c_str();
    } else {
        pathname = "util/tmp.sbe";
        QFile tmpFile(QString::fromStdString(pathname));
        tmpFile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream tmpFileStream(&tmpFile);
        tmpFileStream << ui->SBETextEdit->toPlainText();
        tmpFile.close();
    }

    specVec.at(index)->getSbeFile(pathname, buffer, ARRAYLENGTH);
    specVec.at(index)->writeEeprom(buffer, ARRAYLENGTH, 0);

    delete buffer;

}

void YarrGui::on_SBEReadButton_clicked()
{
    int index = ui->device_comboBox->currentIndex();
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    std::string fnKeyword;
    fnKeyword = ui->filename_keyword->text().toStdString().c_str();

    specVec.at(index)->readEeprom(buffer, ARRAYLENGTH);
    specVec.at(index)->createSbeFile(fnKeyword, buffer, ARRAYLENGTH);

    //TODO make prittier, too much code duplication
    std::stringstream contentStream;


    contentStream << std::hex;
    contentStream << std::showbase;
    contentStream << std::setw(9) << "addr" << std::setw(5) << "msk" << std::setw(12) << "data" << std::endl;
    //256/6 = 42; 256%6 = 4
    {
        uint16_t a;     //address
        uint8_t  m;     //mask
        uint32_t d;     //data
        for(unsigned int i = 0; i<(ARRAYLENGTH / 6); i++) {
            a  = ((buffer[i*6] | (buffer[i*6+1] << 8)) & 0xffc);
            m  = ((buffer[i*6+1] & 0xf0) >> 4);
            d  = (buffer[i*6+2] | (buffer[i*6+3] << 8) | (buffer[i*6+4] << 16) | (buffer[i*6+5] << 24));
            contentStream << std::setw(9) << a << std::setw(5) << (int)m << std::setw(12) << d << std::endl;
        }
    }
    contentStream << std::dec;
    contentStream << std::noshowbase;

    QString content = QString::fromStdString(contentStream.str());

    ui->SBETextEdit->setText(content);

    delete buffer;

}

void YarrGui::on_addFeButton_clicked()
{

    if(!(ui->global_cfg_checkBox->isChecked())){
        unsigned chipIdAdded = (ui->chipIdEdit->text()).toUInt();
        unsigned txChannelAdded = (ui->txChannelEdit->text()).toUInt();
        unsigned rxChannelAdded = (ui->rxChannelEdit->text()).toUInt();

        if(bk->getFe(rxChannelAdded) != NULL) {
            std::cout << "ERROR - rx channel already used. Aborting... \n";
            return;
        }
        bk->addFe(chipIdAdded, txChannelAdded, rxChannelAdded);
        bk->getLastFe()->fromFileBinary((ui->configfileName->text()).toStdString());
        tx->setCmdEnable(0x1 << bk->getLastFe()->getTxChannel());
        bk->getLastFe()->configure();
        bk->getLastFe()->configurePixels();
        while(!(tx->isCmdEmpty())) {
            ;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        tx->setCmdEnable(bk->getTxMask());
        rx->setRxEnable(bk->getRxMask());

        QTreeWidgetItem * feTreeItem = new QTreeWidgetItem(ui->feTree);
        QTreeWidgetItem * feTreeItemId = new QTreeWidgetItem();
        QTreeWidgetItem * feTreeItemTx = new QTreeWidgetItem();
        QTreeWidgetItem * feTreeItemRx = new QTreeWidgetItem();
        QTreeWidgetItem * feTreeItemCf = new QTreeWidgetItem();
        QTreeWidgetItem * feTreeItemCk = new QTreeWidgetItem();

        feTreeItem->setText(0, "FE " + QString::number(ui->feTree->topLevelItemCount()));
        feTreeItemId->setText(0, "Chip ID");
        feTreeItemId->setText(1, QString::number(chipIdAdded));
        feTreeItemTx->setText(0, "TX Channel");
        feTreeItemTx->setText(1, QString::number(txChannelAdded));
        feTreeItemRx->setText(0, "RX Channel");
        feTreeItemRx->setText(1, QString::number(rxChannelAdded));
        feTreeItemCf->setText(0, "Config file");
        feTreeItemCf->setText(1, ui->configfileName->text());
        feTreeItemCk->setText(0, "Scan");
        feTreeItemCk->setFlags(feTreeItemCk->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        feTreeItemCk->setCheckState(1, Qt::Unchecked);

        feTreeItem->addChild(feTreeItemId);
        feTreeItem->addChild(feTreeItemTx);
        feTreeItem->addChild(feTreeItemRx);
        feTreeItem->addChild(feTreeItemCf);
        feTreeItem->addChild(feTreeItemCk);
    } else {
        std::string gCfgFilename((ui->configfileName_2->text()).toStdString());
        std::ifstream gCfg(gCfgFilename);
        if(!gCfg) {
            std::cout << "ERROR - could not open global config file. Aborting... \n";
        }
        std::string chipNameTmp;
        unsigned chipIdTmp;
        unsigned txChannelTmp;
        unsigned rxChannelTmp;
        std::string chipCfgFilenameTmp;

        std::vector<std::string> chipNamesAdded;
        std::vector<unsigned> chipIdsAdded;
        std::vector<unsigned> txChannelsAdded;
        std::vector<unsigned> rxChannelsAdded;
        std::vector<std::string> chipCfgFilenamesAdded;
        char peeek;
        while(!(gCfg.eof()) && gCfg){
            peeek = gCfg.peek();
            if(peeek == '#' || peeek == '\n') {
                char tmpStr[2048];
                gCfg.getline(tmpStr, 2048);
                while(iswspace(gCfg.peek())) {
                    gCfg.get();
                }
                continue;
            }
            gCfg >> chipNameTmp >> chipIdTmp >> txChannelTmp >> rxChannelTmp >> chipCfgFilenameTmp;
            chipNamesAdded.push_back(chipNameTmp);
            chipIdsAdded.push_back(chipIdTmp);
            txChannelsAdded.push_back(txChannelTmp);
            rxChannelsAdded.push_back(rxChannelTmp);
            chipCfgFilenamesAdded.push_back(chipCfgFilenameTmp);
            while(iswspace(gCfg.peek())) {
                gCfg.get();
            }
        }
        for(unsigned int i = 0; i < (chipIdsAdded.size()) ; i++) {
            if(bk->getFe(rxChannelsAdded.at(i)) != NULL) {
                std::cout << "ERROR - rx channel " << rxChannelsAdded.at(i) << " already used. Skipping... \n";
                continue;
            }
            bk->addFe(chipIdsAdded.at(i), txChannelsAdded.at(i), rxChannelsAdded.at(i));
            bk->getLastFe()->fromFileBinary(chipCfgFilenamesAdded.at(i));
            tx->setCmdEnable(0x1 << bk->getLastFe()->getTxChannel());
            bk->getLastFe()->configure();
            bk->getLastFe()->configurePixels();
            while(!(tx->isCmdEmpty())) {
                ;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            tx->setCmdEnable(bk->getTxMask());
            rx->setRxEnable(bk->getRxMask());

            QTreeWidgetItem * feTreeItem = new QTreeWidgetItem(ui->feTree);
            QTreeWidgetItem * feTreeItemId = new QTreeWidgetItem();
            QTreeWidgetItem * feTreeItemTx = new QTreeWidgetItem();
            QTreeWidgetItem * feTreeItemRx = new QTreeWidgetItem();
            QTreeWidgetItem * feTreeItemCf = new QTreeWidgetItem();
            QTreeWidgetItem * feTreeItemCk = new QTreeWidgetItem();

            feTreeItem->setText(0, QString::fromStdString(chipNamesAdded.at(i)));
            feTreeItemId->setText(0, "Chip ID");
            feTreeItemId->setText(1, QString::number(chipIdsAdded.at(i)));
            feTreeItemTx->setText(0, "TX Channel");
            feTreeItemTx->setText(1, QString::number(txChannelsAdded.at(i)));
            feTreeItemRx->setText(0, "RX Channel");
            feTreeItemRx->setText(1, QString::number(rxChannelsAdded.at(i)));
            feTreeItemCf->setText(0, "Config file");
            feTreeItemCf->setText(1, QString::fromStdString(chipCfgFilenamesAdded.at(i)));
            feTreeItemCk->setText(0, "Scan");
            feTreeItemCk->setFlags(feTreeItemCk->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
            feTreeItemCk->setCheckState(1, Qt::Unchecked);

            feTreeItem->addChild(feTreeItemId);
            feTreeItem->addChild(feTreeItemTx);
            feTreeItem->addChild(feTreeItemRx);
            feTreeItem->addChild(feTreeItemCf);
            feTreeItem->addChild(feTreeItemCk);
        }
    }

}

void YarrGui::on_feTree_itemClicked(QTreeWidgetItem * item, int column)
{
    if(item->childCount() == 0) {
        return;
    }
    QString chipIdAdded = item->child(0)->text(1);
    QString txChannelAdded = item->child(1)->text(1);
    QString rxChannelAdded = item->child(2)->text(1);

    ui->chipIdEdit->setText(chipIdAdded);
    ui->txChannelEdit->setText(txChannelAdded);
    ui->rxChannelEdit->setText(rxChannelAdded);
}

void YarrGui::on_remFeButton_clicked()
{
    QTreeWidgetItem * itemRemoved = ui->feTree->currentItem();
    if(itemRemoved->childCount() == 0) {
        return;
    }
    QString channelRemoved = itemRemoved->child(2)->text(1);
    (bk->getFe(channelRemoved.toUInt()))->toFileBinary();
    bk->delFe(channelRemoved.toUInt());
    tx->setCmdEnable(bk->getTxMask());
    rx->setRxEnable(bk->getRxMask());
    delete itemRemoved;
}

void process(Bookkeeper *bookie, bool * scanDone) {
    // Set correct Hit Discriminator setting, for proper decoding
    Fei4DataProcessor proc(bookie->getGlobalFe()->getValue(&Fei4::HitDiscCnfg));
    proc.connect(&bookie->rawData, &bookie->eventMap);
    proc.init();

    while(!(*scanDone)) {
        // TODO some better wakeup signal?
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        proc.process();
    }
    proc.process();
}

void analysis(Fei4Histogrammer *h, Fei4Analysis *a, bool * processorDone) {
    h->init();
    a->init();

    while(!(*processorDone)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        h->process();
        a->process();
    }
    h->process();
    a->process();

    a->end();
}

void YarrGui::doNoiseScan() {
    QString scNm = "NS";
    ScanBase * sc = new Fei4NoiseScan(bk);
    doScan(sc, scNm);
    return;
}

void YarrGui::doDigitalScan()
{
    QString scNm = "DS";
    ScanBase * sc = new Fei4DigitalScan(bk);
    doScan(sc, scNm);
    return;

    /* for(int j = 0; j < ui->feTree->topLevelItemCount(); j++) {
        scanDone = false;
        processorDone = false;
        ScanBase * s = new Fei4DigitalScan(bk);

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        QTreeWidgetItem * plotTreeItem = nullptr;
        for(int k = 0; k < ui->plotTree->topLevelItemCount(); k++) {
           if(ui->plotTree->topLevelItem(k)->text(0) == ui->feTree->topLevelItem(j)->text(0)) {
               plotTreeItem = ui->plotTree->topLevelItem(k);
               break;
           }
        }

        if(plotTreeItem == nullptr) {
            plotTreeItem = new QTreeWidgetItem(ui->plotTree);
            plotTreeItem->setText(0, ui->feTree->topLevelItem(j)->text(0));
        }

        QTreeWidgetItem * plotTreeItemDS = new QTreeWidgetItem();
        plotTreeItemDS->setText(0, "Digitalscan");
        plotTreeItem->addChild(plotTreeItemDS);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
        fe->histogrammer->addHistogrammer(new OccupancyMap());
        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);
        fe->ana->addAlgorithm(new OccupancyAnalysis());

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }

        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

        delete s;
//        fe->toFileBinary();  TODO move to cleanup
//        fe->ana->plot("Digitalscan_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0) {
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            //QString newTabName = "DS FE" + QString::number(j+1);
            QString newTabName = "DS " + ui->feTree->topLevelItem(j)->text(0);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName);

            QTreeWidgetItem * plotTreeItemP = new QTreeWidgetItem();
            plotTreeItemP->setText(0, "Plot " + QString::number(plotTreeItemDS->childCount() + 1));
            plotTreeItemDS->addChild(plotTreeItemP);

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap);
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
                    colorMap->data()->setCell(xCoord, yCoord, colVal);
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel("Occupancy");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

//            QCPMarginGroup * marginGroup = new QCPMarginGroup(tabScanPlot);
//            tabScanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
//            colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();

        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = nullptr;
        delete fe->ana;
        fe->ana = nullptr;

    }

    return; */
}

void YarrGui::doAnalogScan()
{
    QString scNm = "AS";
    ScanBase * sc = new Fei4AnalogScan(bk);
    doScan(sc, scNm);
    return;

    /* for(unsigned j = 0; j < ui->feTree->topLevelItemCount(); j++){
        scanDone = false;
        processorDone = false;
        ScanBase * s = new Fei4AnalogScan(bk);

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
        fe->histogrammer->addHistogrammer(new OccupancyMap());
        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);
        fe->ana->addAlgorithm(new OccupancyAnalysis());

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }
        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

//        tx->setCmdEnable(0x0);
//        rx->setRxEnable(0x0);

        delete s;
        fe->toFileBinary();
//        fe->ana->plot("Analogscan_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0){
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            QString newTabName = "AS FE" + QString::number(j);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName);

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap);
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
                    colorMap->data()->setCell(xCoord, yCoord, colVal);
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel("Occupancy");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();
        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = NULL;
        delete fe->ana;
        fe->ana = NULL;
    }

    return; */
}

void YarrGui::doThresholdScan()
{
    QString scNm = "TS";
    ScanBase * sc = new Fei4ThresholdScan(bk);
    doScan(sc, scNm);
    return;
    /* for(int j = 0; j < ui->feTree->topLevelItemCount(); j++){
        scanDone = false;
        processorDone = false;
        ScanBase * s = new Fei4ThresholdScan(bk);

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
        fe->histogrammer->addHistogrammer(new OccupancyMap());
        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);
        fe->ana->addAlgorithm(new ScurveFitter());

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }
        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

//        tx->setCmdEnable(0x0);
//        rx->setRxEnable(0x0);

        delete s;
        fe->toFileBinary();
        //fe->ana->plot("Thresholdscan_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0){
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            QString newTabName = "TS FE" + QString::number(j);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName);

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap);
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            std::cout << (showMe->getType()).name() << std::endl; //DEBUG
            continue; //DEBUG
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
                    colorMap->data()->setCell(xCoord, yCoord, colVal);
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel("Occupancy");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();
        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = NULL;
        delete fe->ana;
        fe->ana = NULL;
    }

    return; */
}

void YarrGui::doToTScan()
{
    QString scNm = "ToTS";
    ScanBase * sc = new Fei4TotScan(bk);
    doScan(sc, scNm);
    return;

    /* for(int j = 0; j < ui->feTree->topLevelItemCount(); j++){
        scanDone = false;
        processorDone = false;
        ScanBase * s = new Fei4TotScan(bk);

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
        fe->histogrammer->addHistogrammer(new OccupancyMap());
        fe->histogrammer->addHistogrammer(new TotMap());
        fe->histogrammer->addHistogrammer(new Tot2Map());
        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);
        fe->ana->addAlgorithm(new TotAnalysis());

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }
        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

//        tx->setCmdEnable(0x0);
//        rx->setRxEnable(0x0);

        delete s;
        fe->toFileBinary();
        //fe->ana->plot("ToTscan_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0){
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            QString newTabName = "ToTS FE" + QString::number(j);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName);

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap);
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
                    colorMap->data()->setCell(xCoord, yCoord, colVal);
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel("Occupancy");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();
        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = NULL;
        delete fe->ana;
        fe->ana = NULL;
    }

    return; */
}

void YarrGui::doGThrTune()
{
    QString scNm = "GTT";
    ScanBase * sc = new Fei4GlobalThresholdTune(bk);
    doScan(sc, scNm);
    return;

    /* for(int j = 0; j < ui->feTree->topLevelItemCount(); j++){
        scanDone = false;
        processorDone = false;
        ScanBase * s = new Fei4GlobalThresholdTune(bk);

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
        fe->histogrammer->addHistogrammer(new OccupancyMap());
        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);
        fe->ana->addAlgorithm(new OccGlobalThresholdTune());

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }
        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

//        tx->setCmdEnable(0x0);
//        rx->setRxEnable(0x0);

        delete s;
        fe->toFileBinary();
        //fe->ana->plot("GThrTune_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0){
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            QString newTabName = "GTT FE" + QString::number(j);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName);

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap);
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
                    colorMap->data()->setCell(xCoord, yCoord, colVal);
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel("Global Threshold");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();
        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = NULL;
        delete fe->ana;
        fe->ana = NULL;
    }

    return; */
}

void YarrGui::doGPreaTune()
{
    QString scNm = "GPT";
    ScanBase * sc = new Fei4GlobalPreampTune(bk);
    doScan(sc, scNm);
    return;

    /* for(int j = 0; j < ui->feTree->topLevelItemCount(); j++){
        scanDone = false;
        processorDone = false;
        ScanBase * s = new Fei4GlobalPreampTune(bk);

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
        fe->histogrammer->addHistogrammer(new OccupancyMap());
        fe->histogrammer->addHistogrammer(new TotMap());
        fe->histogrammer->addHistogrammer(new Tot2Map());
        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);
        fe->ana->addAlgorithm(new TotAnalysis());

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }
        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

//        tx->setCmdEnable(0x0);
//        rx->setRxEnable(0x0);

        delete s;
        fe->toFileBinary();
        //fe->ana->plot("GPreaTune_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0){
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            QString newTabName = "GPT FE" + QString::number(j);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName);

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap);
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
                    colorMap->data()->setCell(xCoord, yCoord, colVal);
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel("Global Preamp");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();
        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = NULL;
        delete fe->ana;
        fe->ana = NULL;
    }

    return; */
}

void YarrGui::doPThrTune()
{
    QString scNm = "PTT";
    ScanBase * sc = new Fei4PixelThresholdTune(bk);
    doScan(sc, scNm);
    return;

    /* for(int j = 0; j < ui->feTree->topLevelItemCount(); j++){
        scanDone = false;
        processorDone = false;
        ScanBase * s = new Fei4PixelThresholdTune(bk);

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
        fe->histogrammer->addHistogrammer(new OccupancyMap());
        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);
        fe->ana->addAlgorithm(new OccPixelThresholdTune());

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }
        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

//        tx->setCmdEnable(0x0);
//        rx->setRxEnable(0x0);

        delete s;
        fe->toFileBinary();
        //fe->ana->plot("PThrTune_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0){
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            QString newTabName = "PTT FE" + QString::number(j);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName);

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap);
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
                    colorMap->data()->setCell(xCoord, yCoord, colVal);
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel("Pixel Threshold");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();
        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = NULL;
        delete fe->ana;
        fe->ana = NULL;
    }

    return; */
}


void YarrGui::doPPreaTune()
{
    QString scNm = "PPT";
    ScanBase * sc = new Fei4PixelPreampTune(bk);
    doScan(sc, scNm);
    return;

    /* for(int j = 0; j < ui->feTree->topLevelItemCount(); j++){
        scanDone = false;
        processorDone = false;
        ScanBase * s = new Fei4PixelPreampTune(bk);

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
        fe->histogrammer->addHistogrammer(new OccupancyMap());
        fe->histogrammer->addHistogrammer(new TotMap());
        fe->histogrammer->addHistogrammer(new Tot2Map());
        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);
        fe->ana->addAlgorithm(new TotAnalysis());

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }
        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

//        tx->setCmdEnable(0x0);
//        rx->setRxEnable(0x0);

        delete s;
        fe->toFileBinary();
        //fe->ana->plot("PPreaTune_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0){
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            QString newTabName = "PPT FE" + QString::number(j);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName); //ADDED HERE

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap); //ADDED HERE
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord); //TODO MAKE BETTER
                    colorMap->data()->setCell(xCoord, yCoord, colVal);
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale); //ADDED HERE
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel("Pixel Preamp");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();
        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = NULL;
        delete fe->ana;
        fe->ana = NULL;
    }

    return; */
}

void YarrGui::doScan(ScanBase * s, QString qn) {
    for(int j = 0; j < ui->feTree->topLevelItemCount(); j++) {
        scanDone = false;
        processorDone = false;

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; }

        Fei4 * fe = bk->feList.at(j);

        QTreeWidgetItem * plotTreeItem = nullptr;
        for(int k = 0; k < ui->plotTree->topLevelItemCount(); k++) {
           if(ui->plotTree->topLevelItem(k)->text(0) == ui->feTree->topLevelItem(j)->text(0)) {
               plotTreeItem = ui->plotTree->topLevelItem(k);
               break;
           }
        }

        if(plotTreeItem == nullptr) {
            plotTreeItem = new QTreeWidgetItem(ui->plotTree);
            plotTreeItem->setText(0, ui->feTree->topLevelItem(j)->text(0));
        }

        QTreeWidgetItem * plotTreeItemDS = new QTreeWidgetItem();
        plotTreeItemDS->setText(0, qn);
        plotTreeItem->addChild(plotTreeItemDS);

        fe->histogrammer = new Fei4Histogrammer();
        fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);

        fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
        fe->ana->connect(s, fe->clipHisto, fe->clipResult);

        fe->histogrammer->addHistogrammer(new OccupancyMap());

        if(qn == "ToTS" || qn == "GPT" || qn == "PPT") {
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

        s->init();
        s->preScan();

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
        s->postScan();
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++) {
            procThreads[i].join();
        }

        processorDone = true;

        for (unsigned i=0; i<anaThreads.size(); i++) {
            anaThreads[i].join();
        }

        delete s;
//        fe->toFileBinary();
//        fe->ana->plot("Scan_GUI");

        //DEBUG begin [plotting]

        while(fe->clipDataFei4->size() != 0) {
            fe->clipDataFei4->popData();
        }

        while(fe->clipHisto->size() != 0) {
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0) {
            HistogramBase * showMe = fe->clipResult->popData();

            QCustomPlot * tabScanPlot = new QCustomPlot();
            QString newTabName = qn + ' ' + ui->feTree->topLevelItem(j)->text(0);
            ui->scanPlots_tabWidget->addTab(tabScanPlot, newTabName);

            QTreeWidgetItem * plotTreeItemP = new QTreeWidgetItem();
            plotTreeItemP->setText(0, "Plot " + QString::number(plotTreeItemDS->childCount() + 1));
            plotTreeItemDS->addChild(plotTreeItemP);

            QCPColorMap * colorMap = new QCPColorMap(tabScanPlot->xAxis, tabScanPlot->yAxis);
            tabScanPlot->addPlottable(colorMap);
            colorMap->data()->setSize(80, 336);
            colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
            for(int xCoord = 0; xCoord<80; xCoord++) {
                for(int yCoord = 0; yCoord<336; yCoord++) {
                    double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord); //TODO make better
                    colorMap->data()->setCell(xCoord, yCoord, colVal);                //TODO catch other graphs
                }
            }
            std::cout << std::endl;
            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel(" ");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

//            QCPMarginGroup * marginGroup = new QCPMarginGroup(tabScanPlot);
//            tabScanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
//            colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();

        }

        //DEBUG end [plotting]

        delete fe->histogrammer;
        fe->histogrammer = nullptr;
        delete fe->ana;
        fe->ana = nullptr;

    }

    return;
}

void YarrGui::on_NoiseScanButton_clicked() {
    scanVec.push_back( [&] () { doNoiseScan(); } );
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "NS ");
}

void YarrGui::on_DigitalScanButton_clicked() {
    scanVec.push_back( [&] () { doDigitalScan(); });
    //scanVec.push_back( [&] () { doScan((new Fei4DigitalScan(bk)), QString{"DS"}); });
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "DS ");
}

void YarrGui::on_AnalogScanButton_clicked() {
    scanVec.push_back( [&] () { doAnalogScan(); });
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "AS ");
}

void YarrGui::on_ThresholdScanButton_clicked() {
    scanVec.push_back( [&] () { doThresholdScan(); });
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "TS ");
}

void YarrGui::on_ToTScanButton_clicked() {
    scanVec.push_back( [&] () { doToTScan(); });
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "ToTS ");
}

void YarrGui::on_GThrTuneButton_clicked() {
    scanVec.push_back( [&] () { doGThrTune(); });
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "GTT ");
}

void YarrGui::on_GPreaTuneButton_clicked() {
    scanVec.push_back( [&] () { doGPreaTune(); });
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "GPT ");
}

void YarrGui::on_PThrTuneButton_clicked() {
    scanVec.push_back( [&] () { doPThrTune(); });
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "PTT ");
}

void YarrGui::on_PPreaTuneButton_clicked() {
    scanVec.push_back( [&] () { doPPreaTune(); });
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "PPT ");
}

void YarrGui::on_doScansButton_clicked()
{
    std::cout << scanVec.size() << std::endl;
    for(unsigned int i = 0; i<scanVec.size(); i++) {
        (scanVec.at(i))();
    }
}

void YarrGui::on_RemoveScans_Button_clicked()
{
    ui->scanVec_lineEdit->setText("");
    scanVec.clear();
}

void YarrGui::on_removePlot_button_clicked()
{
    delete (ui->scanPlots_tabWidget->currentWidget());
    //QCustomPlot * curPlt = (QCustomPlot*) curWid;

    //QCPLayoutElement * delqcp2 = curPlt->plotLayout()->element(0, 1);
    //curPlt->plotLayout()->remove(delqcp2);

    //QCPAbstractPlottable * delqcp1 = curPlt->plottable(0);
    //curPlt->removePlottable(0);

    //delete curPlt;
    //ui->scanPlots_tabWidget->removeTab(curIn); //TODO MAKE BETTER
    //delete delqcp2;
    //delete delqcp1;

//    curPlt->plottable()
    
}

void YarrGui::on_plotTree_itemClicked(QTreeWidgetItem *item, int column)
{
    if(item->childCount() > 0) {
        return;
    }

    bool foundTabPos = false;
    int plotTabPos = 0;
    for(int i = 0; i < ui->plotTree->topLevelItemCount(); i++) {
        for(int j = 0; j < ui->plotTree->topLevelItem(i)->childCount(); j++) {
            for(int k = 0; k < ui->plotTree->topLevelItem(i)->child(j)->childCount(); k++) {
                plotTabPos++;
                if(ui->plotTree->topLevelItem(i)->child(j)->child(k) == item) {
                    foundTabPos = true;
                }
                if(foundTabPos) {break;}
            }
            if(foundTabPos) {break;}
        }
        if(foundTabPos) {break;}
    }

    if(!foundTabPos) {
        std::cout << "Something went wrong\n";
        return;
    } else {
        ui->scanPlots_tabWidget->setCurrentIndex(plotTabPos - 1);
    } //TODO This is extremely ugly. Switch to model based tree sometime!
}
