#ifndef YARRGUI_H
#define YARRGUI_H

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QColor>
#include <QApplication>

#include <string>
#include <vector>
#include <cmath>

#include "qdebugstream.h"
#include "SpecController.h"
#include "Bookkeeper.h"
#include "qcustomplot.h"
#include "TxCore.h"
#include "RxCore.h"
#include "Fei4.h"
#include "ScanBase.h"
#include "Fei4DataProcessor.h"
#include "Fei4Histogrammer.h"
#include "Fei4Analysis.h"
#include "Fei4Scans.h"
#include "DataProcessor.h"

#include <unistd.h>
#include <sys/wait.h>

#include <sstream>
#include <iomanip>

#include <QFile>
#include <QTextStream>

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <QtTest/QTest>

#include <functional>

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

private slots:

    // Spec
    void on_device_comboBox_currentIndexChanged(int index);
    void on_init_button_clicked();
    void on_progfile_button_clicked();
    void on_prog_button_clicked();

    // Benchmark
    void on_minSize_spinBox_valueChanged(int i);
    void on_maxSize_spinBox_valueChanged(int i);
    void on_startWrite_button_clicked();
    void on_startRead_button_clicked();

    void on_sbefile_button_2_clicked();
    void on_SBEWriteButton_clicked();
    void on_SBEReadButton_clicked();

    void on_addFeButton_clicked();
    void on_feTree_itemClicked(QTreeWidgetItem * item, int column);
    void on_remFeButton_clicked();

    void on_NoiseScanButton_clicked();
    void on_DigitalScanButton_clicked();
    void on_AnalogScanButton_clicked();
    void on_ThresholdScanButton_clicked();
    void on_ToTScanButton_clicked();

    void on_GThrTuneButton_clicked();
    void on_GPreaTuneButton_clicked();
    void on_PThrTuneButton_clicked();
    void on_PPreaTuneButton_clicked();

    void on_doScansButton_clicked();

    void on_RemoveScans_Button_clicked();

    void on_removePlot_button_clicked();

    void on_plotTree_itemClicked(QTreeWidgetItem *item, int column);

    void on_scanPlots_tabWidget_tabBarClicked(int index);

private:
    Ui::YarrGui *ui;
    
    QDir devicePath;
    QStringList deviceList;

    QDebugStream *qout;
    QDebugStream *qerr;

    void init();

    std::vector<SpecController*> specVec;

    std::vector<QCPGraph*> writeGraphVec;
    std::vector<QCPGraph*> readGraphVec;

    Bookkeeper * bk;
    TxCore * tx;
    RxCore * rx;

    std::vector< std::function<void()> > scanVec;

    void doScan(ScanBase *s, QString qn);
    void doNoiseScan();
    void doDigitalScan();
    void doAnalogScan();
    void doThresholdScan();
    void doToTScan();

    void doGThrTune();
    void doGPreaTune();
    void doPThrTune();
    void doPPreaTune();

};

#endif // YARRGUI_H
