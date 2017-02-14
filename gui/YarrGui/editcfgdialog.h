#ifndef EDITCFGDIALOG_H
#define EDITCFGDIALOG_H

#include <QByteArray>
#include <QColor>
#include <QComboBox>
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
#include <math.h>
#include <sstream>
#include <string>
#include <utility>

#include "Fei4.h"
#include "fei4reghelper.h"
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
    explicit EditCfgDialog(Fei4 * f, QString cfgFNJ_param = "util/default.json", QWidget * parent = 0);
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
    void clickHandlerGR(int row, int col);
    void on_zoomInButton_clicked();
    void on_zoomOutButton_clicked();
    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void initGRCB(QComboBox*);
    void normalizeHandler(QString q);
    void updateHandlerGR(int);

    void on_gCfgZoomInButton_clicked();
    void on_gCfgZoomOutButton_clicked();

    void on_chipIdSpin_valueChanged(int arg1);
    void on_lCapDSpin_valueChanged(double arg1);
    void on_sCapDSpin_valueChanged(double arg1);
    void on_vcalOffsetDSpin_valueChanged(double arg1);
    void on_vcalSlopeDSpin_valueChanged(double arg1);

    void on_chipNameLine_textChanged(const QString &arg1);

    void on_txChannelSpin_valueChanged(int arg1);

    void on_rxChannelSpin_valueChanged(int arg1);

private:
    QColor grey;
    QColor darkyellow;
    QColor lightyellow;
    QColor darkgreen;
    QColor lightgreen;
    void normalizeGRColors();

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

//Q_DECLARE_METATYPE(Fei4RegHelper)

#endif // EDITCFGDIALOG_H

