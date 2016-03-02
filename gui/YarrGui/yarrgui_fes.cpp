#ifndef YARRGUI_FES_CPP
#define YARRGUI_FES_CPP

void YarrGui::on_addFeButton_clicked(){

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
    }

}

void YarrGui::on_feTree_itemClicked(QTreeWidgetItem * item, int column){
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

void YarrGui::on_remFeButton_clicked(){
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

#endif
