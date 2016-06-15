#ifndef YARRGUI_FES_CPP
#define YARRGUI_FES_CPP

void YarrGui::on_addFeButton_clicked(){

    unsigned chipIdAdded = (ui->chipIdEdit->text()).toUInt();
    unsigned txChannelAdded = (ui->txChannelEdit->text()).toUInt();
    unsigned rxChannelAdded = (ui->rxChannelEdit->text()).toUInt();

    if(bk->getFe(rxChannelAdded) != NULL) {
        std::cout << "ERROR - rx channel already used. Aborting... \n";
        return;
    }
//    bk->addFe(chipIdAdded, txChannelAdded, rxChannelAdded);
    std::string iFNJ = (ui->configfileName->text()).toStdString();
    std::ifstream iFJ(iFNJ);
    nlohmann::json j;
    try{
        iFJ >> j;
    }
    catch(std::invalid_argument){
        std::cerr << "File does not contain a valid configuration. " << std::endl;
        std::cerr << "Loading default config instead... " << std::endl;
        iFJ.close();
        iFJ.open("util/default.js");
        iFJ >> j;
    }
    iFJ.close();
//    cfgByRxMap[rxChannelAdded] = j;
    bk->getLastFe()->fromFileJson(j);
    bk->getLastFe()->fromFileBinary((ui->configfileName->text()).toStdString()); //JSON here!
    tx->setCmdEnable(0x1 << bk->getLastFe()->getTxChannel());
    bk->getLastFe()->configure();
    bk->getLastFe()->configurePixels();
    while(!(tx->isCmdEmpty())){
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
    feTreeItemCk->setCheckState(1, Qt::Checked);

    feTreeItem->addChild(feTreeItemId);
    feTreeItem->addChild(feTreeItemTx);
    feTreeItem->addChild(feTreeItemRx);
    feTreeItem->addChild(feTreeItemCf);
    feTreeItem->addChild(feTreeItemCk);

    feTreeItem->setExpanded(true);

    return;
}

void YarrGui::on_addFeGlobalButton_clicked(){
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
    for(unsigned int i = 0; i < (chipIdsAdded.size()) ; i++){
        if(bk->getFe(rxChannelsAdded.at(i)) != NULL){
            std::cout << "ERROR - rx channel " << rxChannelsAdded.at(i) << " already used. Skipping... \n";
            continue;
        }
        bk->addFe(chipIdsAdded.at(i), txChannelsAdded.at(i), rxChannelsAdded.at(i));
//        bk->getLastFe()->fromFileBinary(chipCfgFilenamesAdded.at(i)); //JSON here
        std::fstream iFJ(chipCfgFilenamesAdded.at(i), std::ios_base::in);
        nlohmann::json j;
        try{
            iFJ >> j;
        }
        catch(std::invalid_argument){
            std::cerr << chipCfgFilenamesAdded.at(i) << " does not contain a valid configuration. " << std::endl;
            std::cerr << "Using default configuration instead. " << std::endl;
            iFJ.close();
            iFJ.open("util/default.js", std::ios_base::in);
            iFJ >> j;

            std::cout << "************************************************" << std::endl;
            std::cout << std::setw(4) << j;
            std::cout << "************************************************" << std::endl;

            iFJ.close();
            iFJ.open(chipCfgFilenamesAdded.at(i), std::ios_base::out);
            iFJ << std::setw(4) << j;
        }
        iFJ.close();
//        cfgByRxMap[rxChannelsAdded.at(i)] = j; //will remain in memory
        bk->getLastFe()->fromFileJson(j);
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
        QTreeWidgetItem * feTreeItemId = new QTreeWidgetItem(feTreeItem);
        QTreeWidgetItem * feTreeItemTx = new QTreeWidgetItem(feTreeItem);
        QTreeWidgetItem * feTreeItemRx = new QTreeWidgetItem(feTreeItem);
        QTreeWidgetItem * feTreeItemCf = new QTreeWidgetItem(feTreeItem);
        QTreeWidgetItem * feTreeItemCk = new QTreeWidgetItem(feTreeItem);

        feTreeItem->setText(0, QString::fromStdString(chipNamesAdded.at(i)));
        feTreeItemId->setText(0, "Chip ID");
        feTreeItemId->setText(1, QString::number(chipIdsAdded.at(i)));
        feTreeItemTx->setText(0, "TX Channel");
        feTreeItemTx->setText(1, QString::number(txChannelsAdded.at(i)));
        feTreeItemRx->setText(0, "RX Channel");
        feTreeItemRx->setText(1, QString::number(rxChannelsAdded.at(i)));
        feTreeItemCf->setText(0, "Config file");
        feTreeItemCf->setText(1, QString::fromStdString(chipCfgFilenamesAdded.at(i)));
        {
            QPushButton * b = new QPushButton("Edit config", this);
            ui->feTree->setItemWidget(feTreeItemCf, 2, b);
            QObject::connect(b, &QPushButton::clicked, this, [=](){
                EditCfgDialog d(bk->getFe(rxChannelsAdded.at(i)), QString::fromStdString(chipCfgFilenamesAdded.at(i)), this);
                d.exec();
            });
        }
        feTreeItemCk->setText(0, "Scan");
        feTreeItemCk->setFlags(feTreeItemCk->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable);
        feTreeItemCk->setCheckState(1, Qt::Checked);

        feTreeItem->addChild(feTreeItemId);
        feTreeItem->addChild(feTreeItemTx);
        feTreeItem->addChild(feTreeItemRx);
        feTreeItem->addChild(feTreeItemCf);
        feTreeItem->addChild(feTreeItemCk);

        feTreeItem->setExpanded(true);
    }

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

    ui->chipIdEdit->setText(chipIdAdded);
    ui->txChannelEdit->setText(txChannelAdded);
    ui->rxChannelEdit->setText(rxChannelAdded);
    ui->configfileName->setText(configFileAdded);

    return;
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
//    (bk->getFe(channelRemoved.toUInt()))->toFileBinary(); //JSON here
    QString cfgFileRemoved = itemRemoved->child(3)->text(1);
    std::string cfgFNJ = cfgFileRemoved.toStdString();
    std::ofstream cfgFJ(cfgFNJ);
    if(!cfgFJ.is_open()){
        std::cerr << "ERROR - cfg file " << cfgFNJ << " could not be opened. " << std::endl;
        std::cerr << "Current configuration will not be written to a file. " << std::endl;
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

void YarrGui::on_configFile_button_clicked(){
    QString filename = QFileDialog::getOpenFileName(this, tr("Select JSON config file"), "./", tr("JSON Config File(*.js)"));

    ui->configfileName->setText(filename);

    return;
}

void YarrGui::on_gConfigFile_button_clicked(){
    QString filename = QFileDialog::getOpenFileName(this, tr("Select global config file"), "./", tr("Global Config File(*.gcfg)"));

    ui->configfileName_2->setText(filename);

    return;
}

#endif
