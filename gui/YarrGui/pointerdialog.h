#ifndef POINTERDIALOG_H
#define POINTERDIALOG_H

#include <QDialog>

#include "editcfgdialog.h"

namespace Ui {
class PointerDialog;
}

class EditCfgDialog;

class PointerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PointerDialog(QWidget *parent = 0);
    ~PointerDialog();

private slots:
    void on_spinBox_valueChanged(int arg1);

    void on_oneModeRadio_clicked();
    void on_rowModeRadio_clicked();
    void on_colModeRadio_clicked();
    void on_allModeRadio_clicked();

    void on_soneModeRadio_clicked();
    void on_srowModeRadio_clicked();
    void on_scolModeRadio_clicked();
    void on_sallModeRadio_clicked();

private:
    Ui::PointerDialog *ui;
    EditCfgDialog *parentCast;
};

#endif // POINTERDIALOG_H
