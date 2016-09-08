#ifndef EEPROMDIALOG_H
#define EEPROMDIALOG_H

#include <iomanip>
#include <iostream>

#include <QDialog>
#include <QFile>

#include "yarrgui.h"

class YarrGui;

namespace Ui {
class EEPROMDialog;
}

class EEPROMDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EEPROMDialog(QWidget *parent = 0);
    ~EEPROMDialog();

private slots:
    void on_wrFromEditButton_clicked();
    void on_SBEReadButton_clicked();
    void on_SBEWriteButton_clicked();
    void on_sbefile_button_2_clicked();
    void on_sbefile_button_4_clicked();

private:
    Ui::EEPROMDialog *ui;

    YarrGui * parentCast;
};

#endif // EEPROMDIALOG_H
