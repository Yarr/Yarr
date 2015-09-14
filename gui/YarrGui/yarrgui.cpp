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

    ui->scanPlot->xAxis->setLabel("Column");
    ui->scanPlot->yAxis->setLabel("Row");
    ui->scanPlot->xAxis->setRange(0, 80);
    ui->scanPlot->yAxis->setRange(0, 336);
    ui->scanPlot->setInteraction(QCP::iRangeDrag, true);
    ui->scanPlot->setInteraction(QCP::iRangeZoom, true);
    ui->scanPlot->setInteraction(QCP::iSelectPlottables, true);
    ui->scanPlot->setInteraction(QCP::iSelectAxes, true);
    ui->scanPlot->legend->setVisible(false);
    ui->scanPlot->legend->setFont(QFont("Helvetica", 9));
    ui->scanPlot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);

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

    secondTurn = false; //DEBUG
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
        feTreeItemCk->setText(0, "Active");
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
                continue;
            }
            gCfg >> chipNameTmp >> chipIdTmp >> txChannelTmp >> rxChannelTmp >> chipCfgFilenameTmp;
            chipNamesAdded.push_back(chipNameTmp);
            chipIdsAdded.push_back(chipIdTmp);
            txChannelsAdded.push_back(txChannelTmp);
            rxChannelsAdded.push_back(rxChannelTmp);
            chipCfgFilenamesAdded.push_back(chipCfgFilenameTmp);
        }
        for(unsigned i = 0; i<chipIdsAdded.size(); i++) {
            if(bk->getFe(rxChannelsAdded.at(i)) != NULL) {
                std::cout << "ERROR - rx channel already used. Aborting... \n";
                return;
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
            feTreeItemCk->setText(0, "Active");
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
    bk->delFe(channelRemoved.toUInt());
    itemRemoved->~QTreeWidgetItem(); //MEMORY LEAK HERE!
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

void YarrGui::on_NoiseScanButton_clicked()
{
    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4NoiseScan(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

    fe->histogrammer = new Fei4Histogrammer();
    fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
    fe->histogrammer->addHistogrammer(new OccupancyMap());
    fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
    fe->ana->connect(s, fe->clipHisto, fe->clipResult);
    fe->ana->addAlgorithm(new NoiseAnalysis());

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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("Noisescan_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Occupancy");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    return;
}

void YarrGui::on_DigitalScanButton_clicked()
{

    if(tx->isCmdEmpty()) {
        std::cout << "Cmd empty\n";
    } else {
        std::cout << "Cmd not empty\n";
    }

    if(tx->isTrigDone()) {
        std::cout << "Trig done\n";
    } else {
        std::cout << "Trig not done\n";
    }

    std::cout << "Beginning scan... \n";


    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4DigitalScan(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

    fe->histogrammer = new Fei4Histogrammer();
    fe->histogrammer->connect(fe->clipDataFei4, fe->clipHisto);
    fe->histogrammer->addHistogrammer(new OccupancyMap());
    fe->ana = new Fei4Analysis(bk, fe->getRxChannel());
    fe->ana->connect(s, fe->clipHisto, fe->clipResult);
    fe->ana->addAlgorithm(new OccupancyAnalysis());

    s->init();
    QTest::qSleep(10);
    if(secondTurn) {

        std::cout << "Success until here. \n";
        return;
    }
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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("Digitalscan_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

//    if(ui->scanPlot->plottable(0) == NULL) {
//        std::cout << "Nothing here.\n";
//    }

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Occupancy");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

//    ui->scanPlot->removePlottable(colorMap);
//    ui->scanPlot->plotLayout()->remove(colorScale);
//    delete colorMap;
//    delete colorScale;
//    delete marginGroup;

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    secondTurn = true;

    if(tx->isCmdEmpty()) {
        std::cout << "Cmd empty\n";
    } else {
        std::cout << "Cmd not empty\n";
    }

    if(tx->isTrigDone()) {
        std::cout << "Trig done\n";
    } else {
        std::cout << "Trig not done\n";
    }

    std::cout << "Trig abort...\n";
    tx->setTrigCnt(0);

    if(tx->isCmdEmpty()) {
        std::cout << "Cmd empty\n";
    } else {
        std::cout << "Cmd not empty\n";
    }

    if(tx->isTrigDone()) {
        std::cout << "Trig done\n";
    } else {
        std::cout << "Trig not done\n";
    }

    std::cout << std::hex << std::showbase << tx->getTrigEnable() << std::dec << std::noshowbase << std::endl;

    std::cout << "Returning...\n";

    return;
}

void YarrGui::on_AnalogScanButton_clicked()
{
    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4AnalogScan(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("Analogscan_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Occupancy");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    return;
}

void YarrGui::on_ThresholdScanButton_clicked()
{
    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4ThresholdScan(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("Thresholdscan_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Occupancy");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    return;
}

void YarrGui::on_ToTScanButton_clicked()
{
    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4TotScan(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("ToTscan_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Occupancy");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    return;
}

void YarrGui::on_GThrTuneButton_clicked()
{
    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4GlobalThresholdTune(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("GThrTune_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Global Threshold");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    return;
}

void YarrGui::on_GPreaTuneButton_clicked()
{
    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4GlobalPreampTune(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("GPreaTune_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Global Preamp");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    return;
}

void YarrGui::on_PThrTuneButton_clicked()
{
    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4PixelThresholdTune(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("PThrTune_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Pixel Threshold");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    return;
}


void YarrGui::on_PPreaTuneButton_clicked()
{
    scanDone = false;
    processorDone = false;
    ScanBase * s = new Fei4PixelPreampTune(bk);

    Fei4 * fe = NULL;
    for(unsigned i = 0; i < ui->feTree->topLevelItemCount(); i++) {
        if(ui->feTree->topLevelItem(i)->child(4)->checkState(1)) {
            fe = bk->feList[i];
            break;
        }
    }

    if(fe==NULL) {
        std::cout << "No FE chosen for plotting\n";
        return;
    }

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

    tx->setCmdEnable(0x0);
    rx->setRxEnable(0x0);

    delete s;
    fe->toFileBinary();
    fe->ana->plot("PPreaTune_GUI");

    //DEBUG begin [plotting]

    std::deque<HistogramBase*>::iterator it = fe->clipResult->begin();
    it++;
    HistogramBase * showMe = *it;

    QCPColorMap * colorMap = new QCPColorMap(ui->scanPlot->xAxis, ui->scanPlot->yAxis);
    ui->scanPlot->addPlottable(colorMap);
    colorMap->data()->setSize(80, 336);
    colorMap->data()->setRange(QCPRange(0, 80), QCPRange(0, 336));
    for(int xCoord = 0; xCoord<80; xCoord++) {
        for(int yCoord = 0; yCoord<336; yCoord++) {
            double colVal = ((Histo2d *)showMe)->getBin(yCoord + 336*xCoord);
            colorMap->data()->setCell(xCoord, yCoord, colVal);
        }
    }
    std::cout << std::endl;
    QCPColorScale * colorScale = new QCPColorScale(ui->scanPlot);
    ui->scanPlot->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorScale->axis()->setLabel("Pixel Preamp");
    colorMap->setGradient(QCPColorGradient::gpPolar);
    colorMap->rescaleDataRange();

    QCPMarginGroup * marginGroup = new QCPMarginGroup(ui->scanPlot);
    ui->scanPlot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
    ui->scanPlot->rescaleAxes();
    ui->scanPlot->replot();

    //DEBUG end [plotting]

    delete fe->histogrammer;
    fe->histogrammer = NULL;
    delete fe->ana;
    fe->ana = NULL;

    return;
}
