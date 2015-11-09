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

    ui->global_cfg_checkBox->setChecked(true); //DEBUG
    ui->configfileName_2->setText("util/global_config.gcfg"); //DEBUG

    // Init console
    qout = new QDebugStream(std::cout, ui->console, QColor("black"));
    qerr = new QDebugStream(std::cerr, ui->console, QColor("red"));

    ui->main_tabWidget->setCurrentIndex(0);
    ui->main_tabWidget->setTabEnabled(0, true);
    ui->main_tabWidget->setTabEnabled(1, false);
    ui->main_tabWidget->setTabEnabled(2, false);
    ui->main_tabWidget->setTabEnabled(3, false);
    ui->addFuncButton->setEnabled(false);

    ui->feTree->setColumnWidth(0, 200);
    ui->feTree->setColumnWidth(1, 500);
    //ui->feTree->setColumnWidth(2, 2000);

    ui->scanVec_lineEdit->setReadOnly(true);

    ui->additionalFunctionality->addItem("");
    ui->additionalFunctionality->addItem("Benchmark");
    ui->additionalFunctionality->addItem("EEPROM");
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

int YarrGui::getDeviceListSize() {
    return deviceList.size();
}

bool YarrGui::isSpecInitialized(unsigned int i) {
    return specVec.at(i)->isInitialized();
}

SpecController * YarrGui::specVecAt(unsigned int i) {
    return specVec.at(i);
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
            ui->addFuncButton->setEnabled(true);
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

void YarrGui::on_sbefile_button_2_clicked() {
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

void YarrGui::doScan(QString qn) {
    for(int j = 0; j < ui->feTree->topLevelItemCount(); j++) {
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

        if(s == nullptr) {
            std::cerr << "Invalid scan QString parameter passed or scan object construction failed. Exiting...\n";
            exit(-1);
        }

        QTreeWidgetItem * plotTreeItem = nullptr;
        for(int k = 0; k < ui->plotTree->topLevelItemCount(); k++) { //Is the current FE already in the tree? ...
           if(ui->plotTree->topLevelItem(k)->text(0) == ui->feTree->topLevelItem(j)->text(0)) {
               plotTreeItem = ui->plotTree->topLevelItem(k);
               break;
           }
        }

        if(plotTreeItem == nullptr) { //... if not: create a branch fot it
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

            QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
            tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
            colorScale->setType(QCPAxis::atRight);
            colorMap->setColorScale(colorScale);
            colorScale->axis()->setLabel(" ");
            colorMap->setGradient(QCPColorGradient::gpPolar);
            colorMap->rescaleDataRange();

            tabScanPlot->rescaleAxes();
            tabScanPlot->replot();

        }

        delete fe->histogrammer;
        fe->histogrammer = nullptr;
        delete fe->ana;
        fe->ana = nullptr;

    }

    return;
}

void YarrGui::on_NoiseScanButton_clicked() {
    scanVec.push_back("NS");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "NS ");
}

void YarrGui::on_DigitalScanButton_clicked() {
    scanVec.push_back("DS");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "DS ");
}

void YarrGui::on_AnalogScanButton_clicked() {
    scanVec.push_back("AS");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "AS ");
}

void YarrGui::on_ThresholdScanButton_clicked() {
    scanVec.push_back("TS");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "TS ");
}

void YarrGui::on_ToTScanButton_clicked() {
    scanVec.push_back("ToTS");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "ToTS ");
}

void YarrGui::on_GThrTuneButton_clicked() {
    scanVec.push_back("GTT");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "GTT ");
}

void YarrGui::on_GPreaTuneButton_clicked() {
    scanVec.push_back("GPT");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "GPT ");
}

void YarrGui::on_PThrTuneButton_clicked() {
    scanVec.push_back("PTT");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "PTT ");
}

void YarrGui::on_PPreaTuneButton_clicked() {
    scanVec.push_back("PPT");
    ui->scanVec_lineEdit->setText((ui->scanVec_lineEdit->text()) + "PPT ");
}

void YarrGui::on_doScansButton_clicked()
{
    for(unsigned int i = 0; i<scanVec.size(); i++) {
        doScan(scanVec.at(i));
    }
}

void YarrGui::on_RemoveScans_Button_clicked() {
    ui->scanVec_lineEdit->setText("");
    scanVec.clear();
}

void YarrGui::removePlot() {
    if(ui->plotTree->currentItem() == nullptr) {
        std::cout << "Please select plot to delete\n";
        return;
    }
    if(ui->plotTree->currentItem()->childCount() > 0) {
        std::cout << "Please select plot to delete\n";
        return;
    }

    delete (ui->scanPlots_tabWidget->currentWidget());
    delete (ui->plotTree->currentItem());

    for(int i = 0; i < ui->plotTree->topLevelItemCount(); i++) {
        if(ui->plotTree->topLevelItem(i)->childCount() == 0) {
            delete (ui->plotTree->topLevelItem(i));
        } else {
            for(int j = 0; j < ui->plotTree->topLevelItem(i)->childCount(); j++) {
                if(ui->plotTree->topLevelItem(i)->child(j)->childCount() == 0)
                    delete (ui->plotTree->topLevelItem(i)->child(j));
            }
        }
    }
    for(int i = 0; i < ui->plotTree->topLevelItemCount(); i++) {
        if(ui->plotTree->topLevelItem(i)->childCount() == 0) {
            delete (ui->plotTree->topLevelItem(i));
        }
    }

    return;
}

void YarrGui::on_removePlot_button_clicked() {
    removePlot();
    if(ui->plotTree->topLevelItemCount() > 0) {
        ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(0));
    }
    return;
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
        return;
    } else {
        ui->scanPlots_tabWidget->setCurrentIndex(plotTabPos - 1);
    } //TODO This is extremely ugly. Switch to model based tree sometime!
}

void YarrGui::on_scanPlots_tabWidget_tabBarClicked(int index)
{
    int tmpIndex = 0;
    for(int i = 0; i < ui->plotTree->topLevelItemCount(); i++) {
        for(int j = 0; j < ui->plotTree->topLevelItem(i)->childCount(); j++) {
            for(int k = 0; k < ui->plotTree->topLevelItem(i)->child(j)->childCount(); k++) {
                if(tmpIndex == index) {
                    ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(i)->child(j)->child(k));
                }
                tmpIndex++;
            }
        }
    } //TODO This is extremely ugly. Switch to model based tree sometime!
}

void YarrGui::detachPlot() {
    if(ui->plotTree->currentItem() == nullptr) {
        std::cout << "Please select plot to detach...\n";
        return;
    }
    if(ui->plotTree->currentItem()->childCount() > 0) {
        std::cout << "Please select plot to detach...\n";
        return;
    }

    PlotDialog * myPDiag = new PlotDialog();
    QCustomPlot * plotWidget = dynamic_cast<QCustomPlot*>(ui->scanPlots_tabWidget->currentWidget());
    QCPColorMap * widgetCMap = dynamic_cast<QCPColorMap*>(plotWidget->plottable(0));
    if(plotWidget == nullptr || widgetCMap == nullptr) {
        std::cerr << "Dynamic cast failed ): Aborting...\n";
        return;
    }

    QCustomPlot * transferPlot = dynamic_cast<QCustomPlot*>(myPDiag->childAt(10, 10));
    if(transferPlot == nullptr) {
        std::cerr << "Dynamic cast failed. Aborting...\n";
        return;
    }

    QCPColorMap * transferCMap = new QCPColorMap(transferPlot->xAxis, transferPlot->yAxis);
    transferPlot->addPlottable(transferCMap);
    transferCMap->data()->setSize(80, 336);
    transferCMap->setData(widgetCMap->data(), true);
    QCPColorScale * transferCScale = new QCPColorScale(transferPlot);
    transferPlot->plotLayout()->addElement(0, 1, transferCScale);
    transferCScale->setType(QCPAxis::atRight);
    transferCMap->setColorScale(transferCScale);
    transferCScale->axis()->setLabel(" ");
    transferCMap->setGradient(QCPColorGradient::gpPolar);
    transferCMap->rescaleDataRange();

    transferPlot->rescaleAxes();
    transferPlot->replot();

    myPDiag->setModal(false);
    myPDiag->show();

    removePlot();

    return;
}

void YarrGui::on_detachPlot_button__clicked() {
    detachPlot();
    if(ui->plotTree->topLevelItemCount() > 0) {
        ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(0));
    }
    return;
}

void YarrGui::on_detachAll_button_clicked() {

    while(true) {
        ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(0)->child(0)->child(0));
        ui->scanPlots_tabWidget->setCurrentIndex(0);
        detachPlot();
        if(ui->plotTree->topLevelItemCount() == 0) {break;}
    }

    return;
}

void YarrGui::on_debugScanButton_clicked()
{
    QString qn = "Debug";
    for(int j = 0; j < ui->feTree->topLevelItemCount(); j++) {
        scanDone = false;
        processorDone = false;

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))) { continue; } //Is the Scan checkbox checked?

        QTreeWidgetItem * plotTreeItem = nullptr;
        for(int k = 0; k < ui->plotTree->topLevelItemCount(); k++) { //Is the current FE already in the tree? ...
           if(ui->plotTree->topLevelItem(k)->text(0) == ui->feTree->topLevelItem(j)->text(0)) {
               plotTreeItem = ui->plotTree->topLevelItem(k);
               break;
           }
        }

        if(plotTreeItem == nullptr) { //... if not: create a branch fot it
            plotTreeItem = new QTreeWidgetItem(ui->plotTree);
            plotTreeItem->setText(0, ui->feTree->topLevelItem(j)->text(0));
        }

        QTreeWidgetItem * plotTreeItemDS = new QTreeWidgetItem();
        plotTreeItemDS->setText(0, qn);
        plotTreeItem->addChild(plotTreeItemDS);

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
                colorMap->data()->setCell(xCoord, yCoord, 25);
            }
        }

        QCPColorScale * colorScale = new QCPColorScale(tabScanPlot);
        tabScanPlot->plotLayout()->addElement(0, 1, colorScale);
        colorScale->setType(QCPAxis::atRight);
        colorMap->setColorScale(colorScale);
        colorScale->axis()->setLabel(" ");
        colorMap->setGradient(QCPColorGradient::gpPolar);
        colorMap->rescaleDataRange();

        tabScanPlot->rescaleAxes();
        tabScanPlot->replot();

    }

    return;
}

void YarrGui::on_addFuncButton_clicked()
{
    if(ui->additionalFunctionality->currentText() == "Benchmark") {
        BenchmarkDialog * myDialog = new BenchmarkDialog(this);
        myDialog->setModal(false);
        myDialog->setWindowTitle("Benchmark");
        myDialog->show();
    }
    return;
}
