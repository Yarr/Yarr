#ifndef EDITCFGDIALOG_H
#define EDITCFGDIALOG_H

#include <QByteArray>
#include <QColor>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QKeyEvent>
#include <QString>
#include <QTableWidget>
#include <QTextStream>

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "Fei4.h"
#include "json.hpp"
#include "pointerdialog.h"

namespace Ui {
class EditCfgDialog;
}

class PointerDialog;

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
    void clickHandler(int rowCl, int colCl);
    void enterHandler(int rowEn, int colEn);
    void on_zoomInButton_clicked();
    void on_zoomOutButton_clicked();
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

private:
    Ui::EditCfgDialog *ui;

    Fei4 * fE;
    QString cfgFNJ;

    nlohmann::json j;

    QColor* fetchColor(unsigned int v, unsigned int m);
    std::pair<std::string, unsigned int> getMaxVal();
    void initLegend();

    enum class editMode {ONE, ROW, COL, ALL, SONE, SROW, SCOL, SALL};
    editMode eM; //edit mode
    int eN; //edit number
    PointerDialog * pD;

    friend class PointerDialog;
};

#endif // EDITCFGDIALOG_H
