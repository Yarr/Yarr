#ifndef BENCHMARKDIALOG_H
#define BENCHMARKDIALOG_H

#include <QDialog>
#include <QMessageBox>
#include <vector>

#include "BenchmarkTools.h"
#include "qcustomplot.h"
#include "yarrgui.h"

namespace Ui {
class BenchmarkDialog;
}

class YarrGui;

class BenchmarkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BenchmarkDialog(QWidget *parent = 0);
    ~BenchmarkDialog();

private slots:
    void on_startWrite_button_clicked();
    void on_startRead_button_clicked();

private:
    Ui::BenchmarkDialog *ui;
    YarrGui * parentCast;

    std::vector<QCPGraph*> writeGraphVec;
    std::vector<QCPGraph*> readGraphVec;

};

#endif // BENCHMARKDIALOG_H
