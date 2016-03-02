#ifndef YARRGUI_PLOTS_CPP
#define YARRGUI_PLOTS_CPP

void YarrGui::removePlot() {
    if(ui->plotTree->currentItem() == nullptr) {
        std::cerr << "Please select plot to delete\n";
        return;
    }
    if(ui->plotTree->currentItem()->childCount() > 0) {
        std::cerr << "Please select plot to delete\n";
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

void YarrGui::on_removeAllButton_clicked() {
    while(true) {
        ui->plotTree->setCurrentItem(ui->plotTree->topLevelItem(0)->child(0)->child(0));
        ui->scanPlots_tabWidget->setCurrentIndex(0);
        removePlot();
        if(ui->plotTree->topLevelItemCount() == 0) {break;}
    }

    return;
}

void YarrGui::on_plotTree_itemClicked(QTreeWidgetItem *item, int column) {
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

void YarrGui::on_scanPlots_tabWidget_tabBarClicked(int index) {
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
        std::cerr << "Please select plot to detach...\n";
        return;
    }
    if(ui->plotTree->currentItem()->childCount() > 0) {
        std::cerr << "Please select plot to detach...\n";
        return;
    }

    PlotDialog * myPDiag = new PlotDialog();
    QCustomPlot * plotWidget = dynamic_cast<QCustomPlot*>(ui->scanPlots_tabWidget->currentWidget());
    if(plotWidget == nullptr) {
        std::cerr << "Severe cast error. Aborting...\n";
        return;
    }

    QCustomPlot * transferPlot = dynamic_cast<QCustomPlot*>(myPDiag->childAt(10, 10));
    if(transferPlot == nullptr) {
        std::cerr << "Severe cast error. Aborting...\n";
        return;
    }

    QCPPlotTitle * widgetPT = dynamic_cast<QCPPlotTitle*>(plotWidget->plotLayout()->element(0, 0));
    if(widgetPT == nullptr) {
        std::cerr << "Severe cast error. Aborting... \n";
        return;
    }

    if(dynamic_cast<QCPColorMap*>(plotWidget->plottable(0)) != nullptr) {
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
    } else if(dynamic_cast<QCPBars*>(plotWidget->plottable(0)) != nullptr) {
        QCPBars * widgetBars = dynamic_cast<QCPBars*>(plotWidget->plottable(0));

        QCPBars * transferBars = new QCPBars(transferPlot->xAxis, transferPlot->yAxis);
        transferBars->setData(widgetBars->data(), true);
        transferBars->rescaleAxes();

        transferPlot->plotLayout()->insertRow(0);
        transferPlot->plotLayout()->addElement(0, 0, new QCPPlotTitle(transferPlot, widgetPT->text()));
        transferBars->keyAxis()->setLabel(widgetBars->keyAxis()->label());
        transferBars->valueAxis()->setLabel(widgetBars->valueAxis()->label());
    } else {
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

#endif
