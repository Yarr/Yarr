#include "yarrgui.h"
#include "ui_yarrgui.h"

//######################################################################
// CONTENTS:
// -constructor/destructor
// -SPECboard
// -FEs
// -scans
// -plots
// -options menu
//######################################################################

//######################################################################
//####################     contructor/destructor    ####################
//######################################################################

YarrGui::YarrGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::YarrGui)
{
    ui->setupUi(this);

    initSpecLabels();
    initAvailableScans();

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

    ui->configfileName_2->setText("util/global_config.gcfg"); //DEBUG

    // Init console
    qout = new QDebugStream(std::cout, ui->console, QColor("black"));
    qerr = new QDebugStream(std::cerr, ui->console, QColor("red"));

    ui->main_tabWidget->setCurrentIndex(0);
    ui->main_tabWidget->setTabEnabled(0, true);
    ui->main_tabWidget->setTabEnabled(1, false);
    ui->main_tabWidget->setTabEnabled(2, false);

    ui->feTree->setColumnWidth(0, 190);
    ui->feTree->setColumnWidth(1, 150);

    ui->scanVec_lineEdit->setReadOnly(true);

    ui->plotTree->setColumnWidth(0, 500);
    ui->runCustomScanButton->setEnabled(false);
    ui->scanProgressBar->setValue(0);

    QPixmap yarrPapageiPix("yarr_papagei_2.png");
    ui->yarrPapageiLabel->setPixmap(yarrPapageiPix);
    ui->yarrPapageiLabel->setAlignment(Qt::AlignRight);
    ui->yarrPapageiLabel->setScaledContents(true);

    ui->configfileName->setText("util/your_config_here.json");

    ui->addScanButton->setFont(QFont("Sans Serif", 10, QFont::Bold));
    ui->doScansButton->setFont(QFont("Sans Serif", 10, QFont::Bold));

    ui->actionBenchmark->setEnabled(false);
    ui->actionCreate_scan->setEnabled(false);
    ui->actionEEPROM->setEnabled(false);
}

YarrGui::~YarrGui(){
    // Clean up devices
    for(int i=0; i<deviceList.size(); i++) {
        if (specVec[i]->isInitialized())
            delete specVec[i];
    }

    delete ui;
}

//######################################################################
//####################           SPECboard          ####################
//######################################################################

void YarrGui::initSpecLabels() {
    // Preset labels
    ui->specid_value->setNum(-1);
    ui->bar0_value->setNum(-1);
    ui->bar4_value->setNum(-1);
}

void YarrGui::on_device_comboBox_currentIndexChanged(int index) {
    if (specVec.size()) {
        ui->specid_value->setNum(specVec[index]->getId());
        ui->bar0_value->setNum(specVec[index]->getBarSize(0));
        ui->bar4_value->setNum(specVec[index]->getBarSize(4));
    }
}

int YarrGui::getDeviceComboBoxCurrentIndex() {
    return ui->device_comboBox->currentIndex();
}

int YarrGui::getDeviceListSize() {
    return deviceList.size();
}

void YarrGui::on_init_button_clicked(){
    int index = ui->device_comboBox->currentIndex();
    if(specVec.size() == 0 || index > specVec.size()){
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Device not found!");
        return;
    }else{
        specVec[index]->init(index);
        if (specVec[index]->isInitialized()) {
            ui->specid_value->setNum(specVec[index]->getId());
            ui->bar0_value->setNum(specVec[index]->getBarSize(0));
            ui->bar4_value->setNum(specVec[index]->getBarSize(4));
            ui->main_tabWidget->setTabEnabled(1, true);
            ui->main_tabWidget->setTabEnabled(2, true);
            tx = specVec[index];
            rx = specVec[index];
            bk = new Bookkeeper(tx, rx);
        }else{
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Initialization not successful!");
            return;
        }
    }
    ui->actionBenchmark->setEnabled(true);
    ui->actionCreate_scan->setEnabled(true);
    ui->actionEEPROM->setEnabled(true);

    return;
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

//        std::fstream file(ui->progfile_name->text().toStdString().c_str(), std::fstream::in);
        std::fstream file;
        if(ui->revABitfRadio->isChecked()){
            file.open("../hdl/syn/yarr_quad_fei4_revA.bit", std::fstream::in);
        }else if(ui->revBBitfRadio->isChecked()){
            file.open("../hdl/syn/yarr_quad_fei4_revB.bit", std::fstream::in);
        }else{
            std::cerr << "ERROR - must choose one bitfile (rev A or rev B) - aborting... " << std::endl;
            return;
        }
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

SpecController * YarrGui::specVecAt(unsigned int i) {
    return specVec.at(i);
}

bool YarrGui::isSpecInitialized(unsigned int i) {
    return specVec.at(i)->isInitialized();
}

//######################################################################
//####################              FEs             ####################
//######################################################################

void YarrGui::on_addFeButton_clicked(){
    std::string iFNJ = (ui->configfileName->text()).toStdString();
    this->addFE(iFNJ);
}

void YarrGui::on_remFeButton_clicked(){
    if(ui->feTree->currentItem() == nullptr){
        std::cerr << "Please select a FE from the tree. Returning... " << std::endl;
        return;
    }
    QTreeWidgetItem * itemRemoved = ui->feTree->currentItem();
    if(itemRemoved->childCount() == 0) {
        std::cerr << "Please select a FE from the tree. Returning... " << std::endl;
        return;
    }
    QString channelRemoved = itemRemoved->child(2)->text(1);
    QString cfgFileRemoved = itemRemoved->child(3)->text(1);
    std::string cfgFNJ = cfgFileRemoved.toStdString();
    std::ofstream cfgFJ(cfgFNJ);
    if(!cfgFJ.is_open()){
        std::cerr << "ERROR - cfg file " << cfgFNJ << " could not be opened. " << std::endl;
        std::cerr << "Current configuration will not be written to a file. " << std::endl;
    }else{
        nlohmann::json j;
        dynamic_cast<Fei4*>(bk->getFe(channelRemoved.toInt()))->toFileJson(j);
        cfgFJ << std::setw(4) << j;
    }
/*    try{
        nlohmann::json j = cfgByRxMap.at(channelRemoved.toUInt());
        cfgFJ << j;
        cfgByRxMap.erase(channelRemoved.toUInt());
    }
    catch(std::out_of_range & oor){
        std::cerr << "ERROR - no JSON object for the given RX channel. " << std::endl;
        std::cerr << "Current configuration will not be written to a file. " << std::endl;
    }*/
    cfgFJ.close();
    bk->delFe(channelRemoved.toUInt());
    tx->setCmdEnable(bk->getTxMask());
    rx->setRxEnable(bk->getRxMask());
    delete itemRemoved;

    return;
}
//GOFROMHERE
void YarrGui::on_configFile_button_clicked(){
    QString filename = QFileDialog::getOpenFileName(this, tr("Select JSON config file"), "./", tr("JSON Config File(*.json)"));

    ui->configfileName->setText(filename);

    return;
}

void YarrGui::on_feTree_itemClicked(QTreeWidgetItem * item, int column){
    if(item->childCount() == 0) {
        return;
    }
    QString chipIdAdded     = item->child(0)->text(1);
    QString txChannelAdded  = item->child(1)->text(1);
    QString rxChannelAdded  = item->child(2)->text(1);
    QString configFileAdded = item->child(3)->text(1);

    ui->configfileName->setText(configFileAdded);

    return;
}

void YarrGui::on_gConfigFile_button_clicked(){
    QString filename = QFileDialog::getOpenFileName(this, tr("Select global config file"), "./", tr("Global Config File(*.gcfg)"));

    ui->configfileName_2->setText(filename);

    return;
}

void YarrGui::on_addFeGlobalButton_clicked(){
    std::string gCfgFilename((ui->configfileName_2->text()).toStdString());
    std::ifstream gCfg(gCfgFilename);
    if(!gCfg) {
        std::cout << "ERROR - could not open global config file. Aborting... " << std::endl;
        return;
    }
    std::string chipCfgFilenameTmp;

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
        gCfg >> chipCfgFilenameTmp;
        chipCfgFilenamesAdded.push_back(chipCfgFilenameTmp);
        while(iswspace(gCfg.peek())) {
            gCfg.get();
        }
    }
    for(unsigned int i = 0; i < (chipCfgFilenamesAdded.size()) ; i++){
        this->addFE(chipCfgFilenamesAdded.at(i));
    }

    return;
}

bool YarrGui::addFE(std::string fN){
    nlohmann::json j;
    std::ifstream iF(fN);
    if(!iF.is_open()){
        std::cerr << "Config file " << fN << " does not exist. Aborting... " << std::endl;
        return false;
    }
    try{
        iF >> j;
    }
    catch(std::invalid_argument){
        std::cerr << "File " << fN << " does not contain a valid configuration. Aborting... " << std::endl;
        iF.close();
        return false;
    }
    try{
        if(bk->isChannelUsed((unsigned int)j["FE-I4B"]["rxChannel"])){
            std::cerr << "Channel " << (unsigned int)j["FE-I4B"]["rxChannel"]
                      << " in config file " << fN
                      << " already used. Aborting... " << std::endl;
            iF.close();
            return false;
        }
    }
    catch(std::domain_error){
        std::cerr << "Missing txChannel/rxChannel field in config file " << fN
                  << ". Aborting... " << std::endl;
        iF.close();
        return false;
    }
    iF.close();
    bk->addFe(new Fei4(bk->tx,
                       (unsigned int)j["FE-I4B"]["txChannel"],
                       (unsigned int)j["FE-I4B"]["rxChannel"]),
              (unsigned int)j["FE-I4B"]["txChannel"],
              (unsigned int)j["FE-I4B"]["rxChannel"]);
    try{
        dynamic_cast<Fei4*>(bk->getLastFe())->fromFileJson(j);
    }
    catch(std::domain_error){
        std::cerr << "Config file " << fN
                  << " does not contain a valid configuration. Aborting... " << std::endl;
        return false;
    }

    tx->setCmdEnable(0x1 << (unsigned int)j["FE-I4B"]["rxChannel"]);
    dynamic_cast<Fei4*>(bk->getFe((unsigned int)j["FE-I4B"]["rxChannel"]))->configure();
    dynamic_cast<Fei4*>(bk->getFe((unsigned int)j["FE-I4B"]["rxChannel"]))->configurePixels();
    while(!(tx->isCmdEmpty())) {
        ;
    }

    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    tx->setCmdEnable(bk->getTxMask());
    rx->setRxEnable(bk->getRxMask());

    QTreeWidgetItem * feTreeItem = new QTreeWidgetItem(ui->feTree);
    QTreeWidgetItem * feTreeItemId = new QTreeWidgetItem(feTreeItem);
    QTreeWidgetItem * feTreeItemTx = new QTreeWidgetItem(feTreeItem);
    QTreeWidgetItem * feTreeItemRx = new QTreeWidgetItem(feTreeItem);
    QTreeWidgetItem * feTreeItemCf = new QTreeWidgetItem(feTreeItem);
    QTreeWidgetItem * feTreeItemCk = new QTreeWidgetItem(feTreeItem);

    {
        std::string chipNameTmp;
        unsigned int chipIdTmp;
        unsigned int txChannelTmp;
        unsigned int rxChannelTmp;
        try{
            chipNameTmp = j["FE-I4B"]["name"];
        }
        catch(std::domain_error){
            std::cerr << "Config file " << fN << " has no \"name\" field. "
                      << "Defaulting to \"myChip\". " << std::endl;
            chipNameTmp = "myChip";
        }
        try{
            chipIdTmp = j["FE-I4B"]["Parameter"]["chipId"];
        }
        catch(std::domain_error){
            std::cerr << "Config file " << fN << " has no \"chipId\" field. "
                      << "Defaulting to 6. " << std::endl;
            chipIdTmp = 6;
        }
        txChannelTmp = j["FE-I4B"]["txChannel"];
        rxChannelTmp = j["FE-I4B"]["rxChannel"];

        feTreeItem->setText(0, QString::fromStdString(chipNameTmp));
        feTreeItemId->setText(0, "Chip ID");
        feTreeItemId->setText(1, QString::number(chipIdTmp));
        feTreeItemTx->setText(0, "TX Channel");
        feTreeItemTx->setText(1, QString::number(txChannelTmp));
        feTreeItemRx->setText(0, "RX Channel");
        feTreeItemRx->setText(1, QString::number(rxChannelTmp));
        feTreeItemCf->setText(0, "Config file");
        feTreeItemCf->setText(1, QString::fromStdString(fN));
    }

    QPushButton * b = new QPushButton("Edit config", this);
    ui->feTree->setItemWidget(feTreeItemCf, 2, b);
    QObject::connect(b, &QPushButton::clicked, this, [=](){
        EditCfgDialog d(dynamic_cast<Fei4*>(bk->getFe((unsigned int)j["FE-I4B"]["rxChannel"])),
                        QString::fromStdString(fN), this);
        d.exec();
    });

    feTreeItemCk->setText(0, "Scan");
    feTreeItemCk->setFlags(feTreeItemCk->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
    feTreeItemCk->setCheckState(1, Qt::Checked);

    feTreeItem->addChild(feTreeItemId);
    feTreeItem->addChild(feTreeItemTx);
    feTreeItem->addChild(feTreeItemRx);
    feTreeItem->addChild(feTreeItemCf);
    feTreeItem->addChild(feTreeItemCk);

    feTreeItem->setExpanded(true);

    return true;
}

//######################################################################
//####################             scans            ####################
//######################################################################

void process(Bookkeeper *bookie, bool * scanDone) {
    // Set correct Hit Discriminator setting, for proper decoding
    Fei4DataProcessor proc(bookie->g_fe->getValue(&Fei4::HitDiscCnfg));
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

void YarrGui::initAvailableScans(){
    availableScansMap.insert("NoiseScan (NS)", "NS");
    availableScansMap.insert("DigitalScan (DS)", "DS");
    availableScansMap.insert("AnalogScan (AS)", "AS");
    availableScansMap.insert("ThresholdScan (TS)", "TS");
    availableScansMap.insert("ToTScan (ToTS)", "ToTS");

    availableScansMap.insert("GlobalThresholdTune (GTT)", "GTT");
    availableScansMap.insert("GlobalPreampTune (GPT)", "GPT");
    availableScansMap.insert("PixelThresholdTune (PTT)", "PTT");
    availableScansMap.insert("PixelPreampTune (PPT)", "PPT");

    QMap<QString, QString>::const_iterator i = availableScansMap.constBegin();
    while(i != availableScansMap.constEnd()){
        ui->addScanBox->addItem(i.key());
        ++i;
    }

    return;
}

void YarrGui::setCustomScan(CustomScan & other) {
    cs = other;
    ui->runCustomScanButton->setEnabled(true);

    return;
}

void YarrGui::doScan(QString qn){
    std::ofstream *tmpOfCout = new std::ofstream("deleteMeCout.txt");
    std::streambuf *coutBuf = std::cout.rdbuf(tmpOfCout->rdbuf());
    std::ofstream *tmpOfCerr = new std::ofstream("deleteMeCerr.txt");
    std::streambuf *cerrBuf = std::cerr.rdbuf(tmpOfCerr->rdbuf());

    int N = ui->feTree->topLevelItemCount();
    int M = scanVec.size();
    for(int j = 0; j < N; j++){
        scanDone = false;
        processorDone = false;

        if(!(ui->feTree->topLevelItem(j)->child(4)->checkState(1))){continue;} //Is the Scan checkbox checked?

        Fei4 * fe = dynamic_cast<Fei4*>(bk->feList.at(j));

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

        if(s == nullptr){
            std::cerr << "Invalid scan QString parameter passed or scan object construction failed. Returning...\n";
            return;
        }

        QTreeWidgetItem * plotTreeItem = nullptr; //Plot tree item for current FE
        for(int k = 0; k < ui->plotTree->topLevelItemCount(); k++){ //Is the current FE already in the tree? ...
           if(ui->plotTree->topLevelItem(k)->text(0) == ui->feTree->topLevelItem(j)->text(0)){
               plotTreeItem = ui->plotTree->topLevelItem(k);
               break;
           }
        }

        if(plotTreeItem == nullptr){ //... if not: create a branch for it
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

        if (qn=="CS"){
            CustomScan * tmp;
            tmp = dynamic_cast<CustomScan*>(s);
            if(tmp->bA.at(OCC_MAP) == true)   {fe->histogrammer->addHistogrammer(new OccupancyMap());}
            if(tmp->bA.at(TOT_MAP) == true)   {fe->histogrammer->addHistogrammer(new TotMap());}
            if(tmp->bA.at(TOT_2_MAP) == true) {fe->histogrammer->addHistogrammer(new Tot2Map());}

            if(tmp->bA.at(OCC_ANA) == true)   {fe->ana->addAlgorithm(new OccupancyAnalysis());}
            if(tmp->bA.at(NOISE_ANA) == true) {fe->ana->addAlgorithm(new NoiseAnalysis());}
            if(tmp->bA.at(TOT_ANA) == true)   {fe->ana->addAlgorithm(new TotAnalysis());}
            if(tmp->bA.at(S_CU_FIT) == true)  {fe->ana->addAlgorithm(new ScurveFitter());}
            if(tmp->bA.at(PIX_THR) == true)   {fe->ana->addAlgorithm(new OccPixelThresholdTune);}
        }else{
            fe->histogrammer->addHistogrammer(new OccupancyMap());

            if (qn == "ToTS" || qn == "GPT" || qn == "PPT"){
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
        for (unsigned i=0; i<numThreads; i++){
            procThreads.push_back(std::thread(process, bk, &scanDone));
            //std::cout << "  -> Processor thread #" << i << " started!" << std::endl;
        }

        std::vector<std::thread> anaThreads;
        //std::cout << "-> Starting histogrammer and analysis threads:" << std::endl;
        if (fe->isActive()){
            anaThreads.push_back(std::thread(analysis, fe->histogrammer, fe->ana, &processorDone));
            //std::cout << "  -> Analysis thread of Fe " << fe->getRxChannel() << std::endl;
        }

        s->run();
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //3
        s->postScan();
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //4
        scanDone = true;

        for (unsigned i=0; i<numThreads; i++){
            procThreads[i].join();
        }
        ui->scanProgressBar->setValue(ui->scanProgressBar->value() + (int)(100.0/(7.0*N*M))); //5

        processorDone = true;

        for(unsigned i=0; i<anaThreads.size(); i++){
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
        while(fe->clipDataFei4->size() != 0){
            fe->clipDataFei4->popData();
        }

        //clear raw data
        while(fe->clipHisto->size() != 0){
            fe->clipHisto->popData();
        }

        while(fe->clipResult->size() != 0){
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

            if(dynamic_cast<Histo2d*>(showMe) != nullptr){
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
                for(int xCoord = 0; xCoord<80; xCoord++){
                    for(int yCoord = 0; yCoord<336; yCoord++){
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

            }else if(dynamic_cast<Histo1d*>(showMe) != nullptr){
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

            }else{
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

    std::cout.rdbuf(coutBuf);
    std::cerr.rdbuf(cerrBuf);
    tmpOfCout->close();
    tmpOfCerr->close();

    std::ifstream tmpInCout("deleteMeCout.txt");
    std::ifstream tmpInCerr("deleteMeCerr.txt");
    std::cout << tmpInCout.rdbuf();
    std::cerr << tmpInCerr.rdbuf();
    tmpInCout.close();
    tmpInCerr.close();

    return;
}

void YarrGui::addScan(QString const& myKey){
    QString myVal = this->availableScansMap.value(myKey);
    this->scanVec.push_back(myVal);
    myVal.append(" ");
    this->ui->scanVec_lineEdit->insert(myVal);
}

void YarrGui::on_addScanButton_clicked(){
    QString myKey = ui->addScanBox->currentText();
    this->addScan(myKey);
}

void YarrGui::on_addScanBox_activated(const QString &arg1){
    this->addScan(arg1);
}

void YarrGui::on_doScansButton_clicked(){
    ui->scanProgressBar->setValue(0);
    unsigned int M = scanVec.size();
    for(unsigned int i = 0; i < M; i++){
        doScan(scanVec.at(i));
        ui->scanProgressBar->setValue(100*(i+1)/M);
    }
    ui->scanProgressBar->setValue(100);
}

void YarrGui::on_RemoveScans_Button_clicked(){
    ui->scanVec_lineEdit->setText("");
    scanVec.clear();
}

void YarrGui::on_runCustomScanButton_clicked(){
    doScan("CS");
    ui->runCustomScanButton->setEnabled(false);

    return;
}

//######################################################################
//####################             plots            ####################
//######################################################################

void YarrGui::removePlot(){
    if(ui->plotTree->currentItem() == nullptr){
        std::cerr << "Please select plot to delete\n";
        return;
    }
    if(ui->plotTree->currentItem()->childCount() > 0){
        std::cerr << "Please select plot to delete\n";
        return;
    }

    delete (ui->scanPlots_tabWidget->currentWidget());
    delete (ui->plotTree->currentItem());

    for(int i = 0; i < ui->plotTree->topLevelItemCount(); i++){
        if(ui->plotTree->topLevelItem(i)->childCount() == 0){
            delete (ui->plotTree->topLevelItem(i));
        }else{
            for(int j = 0; j < ui->plotTree->topLevelItem(i)->childCount(); j++){
                if(ui->plotTree->topLevelItem(i)->child(j)->childCount() == 0){
                    delete (ui->plotTree->topLevelItem(i)->child(j));
                }
            }
        }
    }
    for(int i = 0; i < ui->plotTree->topLevelItemCount(); i++){
        if(ui->plotTree->topLevelItem(i)->childCount() == 0){
            delete (ui->plotTree->topLevelItem(i));
        }
    }

    return;
}

void YarrGui::detachPlot(){
    if(ui->plotTree->currentItem() == nullptr){
        std::cerr << "Please select plot to detach...\n";
        return;
    }
    if(ui->plotTree->currentItem()->childCount() > 0){
        std::cerr << "Please select plot to detach...\n";
        return;
    }

    PlotDialog * myPDiag = new PlotDialog();
    QCustomPlot * plotWidget = dynamic_cast<QCustomPlot*>(ui->scanPlots_tabWidget->currentWidget());
    if(plotWidget == nullptr){
        std::cerr << "Severe cast error. Aborting...\n";
        return;
    }

    QCustomPlot * transferPlot = dynamic_cast<QCustomPlot*>(myPDiag->childAt(10, 10));
    if(transferPlot == nullptr){
        std::cerr << "Severe cast error. Aborting...\n";
        return;
    }

    QCPPlotTitle * widgetPT = dynamic_cast<QCPPlotTitle*>(plotWidget->plotLayout()->element(0, 0));
    if(widgetPT == nullptr){
        std::cerr << "Severe cast error. Aborting... \n";
        return;
    }

    if(dynamic_cast<QCPColorMap*>(plotWidget->plottable(0)) != nullptr){
        QCPColorMap * widgetCMap = dynamic_cast<QCPColorMap*>(plotWidget->plottable(0));

        QCPColorScale * widgetCScale = dynamic_cast<QCPColorScale*>(plotWidget->plotLayout()->element(1, 1));
        if(widgetCScale == nullptr) {
            std::cerr << "Severe cast error. Aborting... \n";
            return;
        }

        transferPlot->plotLayout()->insertRow(0);
        transferPlot->plotLayout()->addElement(0, 0, new QCPPlotTitle(transferPlot, widgetPT->text()));

        QCPColorMap * transferCMap = new QCPColorMap(transferPlot->xAxis, transferPlot->yAxis);
        transferPlot->addPlottable(transferCMap);
        transferCMap->data()->setSize(80, 336);
        transferCMap->setData(widgetCMap->data(), true);
        QCPColorScale * transferCScale = new QCPColorScale(transferPlot);
        transferPlot->plotLayout()->addElement(1, 1, transferCScale);
        transferCScale->setType(QCPAxis::atRight);
        transferCMap->setColorScale(transferCScale);
        transferCMap->keyAxis()->setLabel(widgetCMap->keyAxis()->label());
        transferCMap->valueAxis()->setLabel(widgetCMap->valueAxis()->label());

        transferCScale->axis()->setLabel(widgetCScale->axis()->label());
        transferCMap->setGradient(QCPColorGradient::gpPolar);
        transferCMap->rescaleDataRange();
    }else if(dynamic_cast<QCPBars*>(plotWidget->plottable(0)) != nullptr){
        QCPBars * widgetBars = dynamic_cast<QCPBars*>(plotWidget->plottable(0));

        QCPBars * transferBars = new QCPBars(transferPlot->xAxis, transferPlot->yAxis);
        transferBars->setData(widgetBars->data(), true);
        transferBars->rescaleAxes();

        transferPlot->plotLayout()->insertRow(0);
        transferPlot->plotLayout()->addElement(0, 0, new QCPPlotTitle(transferPlot, widgetPT->text()));
        transferBars->keyAxis()->setLabel(widgetBars->keyAxis()->label());
        transferBars->valueAxis()->setLabel(widgetBars->valueAxis()->label());
    }else{
        std::cerr << "Severe cast error. Aborting... \n";                       //DEBUG
        return;
    }

    transferPlot->rescaleAxes();
    transferPlot->replot();

    myPDiag->setModal(false);
    myPDiag->show();

    removePlot();

    return;
}

void YarrGui::on_plotTree_itemClicked(QTreeWidgetItem *item, int column){
    if(item->childCount() > 0){
        return;
    }

    bool foundTabPos = false;
    int plotTabPos = 0;
    for(int i = 0; i < ui->plotTree->topLevelItemCount(); i++){
        for(int j = 0; j < ui->plotTree->topLevelItem(i)->childCount(); j++){
            for(int k = 0; k < ui->plotTree->topLevelItem(i)->child(j)->childCount(); k++){
                plotTabPos++;
                if(ui->plotTree->topLevelItem(i)->child(j)->child(k) == item){
                    foundTabPos = true;
                }
                if(foundTabPos){break;}
            }
            if(foundTabPos){break;}
        }
        if(foundTabPos){break;}
    }

    if(!foundTabPos){
        return;
    }else{
        ui->scanPlots_tabWidget->setCurrentIndex(plotTabPos - 1);
    } //TODO This is extremely ugly. Switch to model based tree sometime!
}

void YarrGui::on_removePlot_button_clicked(){
    removePlot();
    if(ui->plotTree->topLevelItemCount() > 0){
        ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(0));
    }
    return;
}

void YarrGui::on_removeAllButton_clicked(){
    while(true){
        ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(0)->child(0)->child(0));
        ui->scanPlots_tabWidget->setCurrentIndex(0);
        removePlot();
        if(ui->plotTree->topLevelItemCount() == 0){break;}
    }

    return;
}

void YarrGui::on_detachPlot_button__clicked(){
    detachPlot();
    if(ui->plotTree->topLevelItemCount() > 0){
        ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(0));
    }
    return;
}

void YarrGui::on_detachAll_button_clicked(){
    while(true){
        ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(0)->child(0)->child(0));
        ui->scanPlots_tabWidget->setCurrentIndex(0);
        detachPlot();
        if(ui->plotTree->topLevelItemCount() == 0){break;}
    }
    return;
}

void YarrGui::on_exportPlotButton_clicked(){
    if(ui->scanPlots_tabWidget->count() == 0){return;}
    if(ui->plotTree->currentItem() == nullptr) {
        std::cerr << "Please select plot. Returning... " << std::endl;
        return;
    }
    if(ui->plotTree->currentItem()->childCount() > 0){
        std::cerr << "Please select plot. Returning... " << std::endl;
        return;
    }

    QWidget * toCast = ui->scanPlots_tabWidget->currentWidget();
    QCustomPlot * myPlot = dynamic_cast<QCustomPlot*>(toCast);
    if(myPlot == nullptr){
        std::cerr << "Severe cast error. Returning... " << std::endl;

        return;
    }

/*    QString myFileName = ui->plotTree->currentItem()->text(0);

    struct tm * timeinfo;
    time_t rawtime;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    myFileName = myFileName
               + QString::number(1900+(timeinfo->tm_year)) + '_'
               + (timeinfo->tm_mon > 8 ? QString::number(1+(timeinfo->tm_mon)) : ('0' + QString::number(1+(timeinfo->tm_mon)))) + '_'
               + QString::number((timeinfo->tm_mday)) + '_'
               + QString::number((timeinfo->tm_hour)) + '_'
               + QString::number((timeinfo->tm_min)) + '_'
               + QString::number((timeinfo->tm_sec)) + ".pdf";
*/

    QString myFileName = QFileDialog::getSaveFileName(this,
                                                      "Save plot as PDF",
                                                      "./",
                                                      "Portable Document Format(*.pdf)");

    myPlot->savePdf(myFileName);
    std::cout << "Saved current plot to \"" << myFileName.toStdString() << '"' << std::endl;

    return;
}


void YarrGui::on_exportPlotCSVButton_clicked(){
    if(ui->scanPlots_tabWidget->count() == 0){return;}
    if(ui->plotTree->currentItem() == nullptr) {
        std::cerr << "Please select plot. Returning... " << std::endl;
        return;
    }
    if(ui->plotTree->currentItem()->childCount() > 0){
        std::cerr << "Please select plot. Returning... " << std::endl;
        return;
    }

    QWidget * toCast = ui->scanPlots_tabWidget->currentWidget();
    QCustomPlot * myPlot = dynamic_cast<QCustomPlot*>(toCast);
    if(myPlot == nullptr){
        std::cerr << "Severe cast error. Returning... " << std::endl;
        return;
    }

    QCPColorMap * myColorMap = dynamic_cast<QCPColorMap*>(myPlot->plottable());
    if(myColorMap == nullptr){
        std::cout << "Severe cast error. Aborting... " << std::endl;
        return;
    }

/*    QString myFileName = ui->plotTree->currentItem()->text(0);

    struct tm * timeinfo;
    time_t rawtime;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    myFileName = myFileName
               + QString::number(1900+(timeinfo->tm_year)) + '_'
               + (timeinfo->tm_mon > 8 ? QString::number(1+(timeinfo->tm_mon)) : ('0' + QString::number(1+(timeinfo->tm_mon)))) + '_'
               + QString::number((timeinfo->tm_mday)) + '_'
               + QString::number((timeinfo->tm_hour)) + '_'
               + QString::number((timeinfo->tm_min)) + '_'
               + QString::number((timeinfo->tm_sec)) + ".csv";
*/

    QString myFileName = QFileDialog::getSaveFileName(this,
                                                      "Save plot as CSV",
                                                      "./",
                                                      "Comma-Separated Values(*.csv)");
    if(myFileName==""){return;}

    std::ofstream myCSVOutput(myFileName.toStdString());

    for(int xCoord = 0; xCoord<80; xCoord++){
        for(int yCoord = 0; yCoord<336; yCoord++) {
            myCSVOutput << xCoord << ",\t" << yCoord << ",\t" << myColorMap->data()->cell(xCoord, yCoord) << std::endl;
        }
    }

    myCSVOutput.close();
    std::cout << "Saved current plot to \"" << myFileName.toStdString() << '"' << std::endl;

    return;
}

void YarrGui::on_scanPlots_tabWidget_tabBarClicked(int index){
    int tmpIndex = 0;
    for(int i = 0; i < ui->plotTree->topLevelItemCount(); i++){
        for(int j = 0; j < ui->plotTree->topLevelItem(i)->childCount(); j++){
            for(int k = 0; k < ui->plotTree->topLevelItem(i)->child(j)->childCount(); k++){
                if(tmpIndex == index){
                    ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(i)->child(j)->child(k));
                }
                tmpIndex++;
            }
        }
    } //TODO This is ugly. Switch to model based tree sometime!
}

//######################################################################
//####################         options menu         ####################
//######################################################################

void YarrGui::on_actionBenchmark_triggered(){
    BenchmarkDialog * myDialog = new BenchmarkDialog(this);
    myDialog->setModal(false);
    myDialog->setWindowTitle("Benchmark");
    myDialog->setModal(true);
    myDialog->showMaximized();

    return;
}

void YarrGui::on_actionEEPROM_triggered(){
    EEPROMDialog * myDialog = new EEPROMDialog(this);
    myDialog->setModal(false);
    myDialog->setWindowTitle("SpecBoard EEPROM");
    myDialog->setModal(true);
    myDialog->showMaximized();

    return;
}

void YarrGui::on_actionCreate_scan_triggered(){
    CreateScanDialog * myDialog = new CreateScanDialog(bk, this);
    myDialog->setModal(false);
    myDialog->setWindowTitle("Create scan");
    myDialog->setModal(true);
    myDialog->showMaximized();

    return;
}
