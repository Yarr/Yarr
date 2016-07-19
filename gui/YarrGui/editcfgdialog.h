#ifndef EDITCFGDIALOG_H
#define EDITCFGDIALOG_H

#include <QByteArray>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QString>
#include <QTableWidget>
#include <QTextStream>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Fei4.h"
#include "json.hpp"

namespace Ui {
class EditCfgDialog;
}

class EditCfgDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditCfgDialog(Fei4 * f, QString cfgFNJ_param = "util/default.js", QWidget * parent = 0);
    ~EditCfgDialog();

private slots:
    void on_applyButton_clicked();
    void on_saveButton_clicked();
    void on_saveAsButton_clicked();

    void on_EnableRadio_clicked();
    void on_TDACRadio_clicked();
    void on_LCapRadio_clicked();
    void on_SCapRadio_clicked();
    void on_HitbusRadio_clicked();
    void on_FDACRadio_clicked();

private:
    Ui::EditCfgDialog *ui;

    Fei4 * fE;
    QString cfgFNJ;
};

#endif // EDITCFGDIALOG_H
