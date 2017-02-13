#ifndef YARRGUI_H
#define YARRGUI_H


#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QMainWindow>
#include <QMap>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QTest>
#include <QtTest/QTest>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <BitFile.h>
#include <BenchmarkTools.h>
#include "qdebugstream.h"
#include "SpecController.h"
#include "benchmarkdialog.h"
#include "createscandialog.h"
#include "Bookkeeper.h"
#include "eepromdialog.h"
#include "plotdialog.h"
#include "qcustomplot.h"
#include "SpecTxCore.h"
#include "SpecRxCore.h"
#include "Fei4.h"
#include "ScanBase.h"
#include "scanstruct.h"
#include "editcfgdialog.h"
#include "Fei4DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "Fei4Analysis.h"
#include "Fei4Scans.h"
#include "DataProcessor.h"
#include "json.hpp"

namespace Ui {
class YarrGui;
}

class YarrGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit YarrGui(QWidget *parent = 0);
    ~YarrGui();

    bool scanDone;
    bool processorDone;

    int getDeviceComboBoxCurrentIndex();
    int getDeviceListSize();
    bool isSpecInitialized(unsigned int i);
    SpecController * specVecAt(unsigned int i);

    void setCustomScan(CustomScan & other);

private slots:

    // Spec
    void on_device_comboBox_currentIndexChanged(int index);
    void on_init_button_clicked();
    void on_progfile_button_clicked();
    void on_prog_button_clicked();

    //Fe
    void on_addFeButton_clicked();
    void on_feTree_itemClicked(QTreeWidgetItem * item, int column);
    void on_remFeButton_clicked();
    void on_configFile_button_clicked();
    void on_gConfigFile_button_clicked();

    //Scans
    void on_doScansButton_clicked();
    void on_RemoveScans_Button_clicked();

    void on_plotTree_itemClicked(QTreeWidgetItem *item, int column);
    void on_scanPlots_tabWidget_tabBarClicked(int index);

    void on_detachPlot_button__clicked();
    void on_detachAll_button_clicked();
    void on_removePlot_button_clicked();
    void on_removeAllButton_clicked();
    void on_runCustomScanButton_clicked();

    //void on_debugScanButton_clicked();

    void addScan(QString const& myKey);
    void on_addScanButton_clicked();
    void on_exportPlotButton_clicked();
    void on_addFeGlobalButton_clicked();
    void on_exportPlotCSVButton_clicked();

    //menu bar
    void on_actionBenchmark_triggered();
    void on_actionEEPROM_triggered();
    void on_actionCreate_scan_triggered();

    void on_addScanBox_activated(const QString &arg1);

private:
    bool addFE(std::string);

    Ui::YarrGui * ui;
    
    QDir devicePath;
    QStringList deviceList;

    QDebugStream *qout;
    QDebugStream *qerr;

    std::vector<SpecController*> specVec;

    Bookkeeper * bk;
    SpecTxCore * tx;
    SpecRxCore * rx;

    std::vector<QString> scanVec;
    CustomScan cs;
    QMap<QString, QString> availableScansMap;
//    std::map<unsigned int, nlohmann::json> cfgByRxMap;

    void initAvailableScans();
    void initSpecLabels();
    void doScan(QString qn);
    void detachPlot();
    void removePlot();

};

#endif // YARRGUI_H

