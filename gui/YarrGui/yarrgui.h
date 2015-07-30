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
#include "qcustomplot.h"

#include <unistd.h>
#include <sys/wait.h>

#include <sstream>
#include <iomanip>

#include <QFile>
#include <QTextStream>

namespace Ui {
class YarrGui;
}

class YarrGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit YarrGui(QWidget *parent = 0);
    ~YarrGui();

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
    void on_NoiseScanButton_clicked();

    void on_sbefile_button_2_clicked();

    void on_SBEWriteButton_clicked();

    void on_SBEReadButton_clicked();

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
};

#endif // YARRGUI_H
