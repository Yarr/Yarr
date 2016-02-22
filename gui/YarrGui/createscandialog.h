#ifndef CREATESCANDIALOG_H
#define CREATESCANDIALOG_H

#include <iostream>
#include <math.h>

#include <QComboBox>
#include <QDialog>
#include <QMessageBox>

#include "Bookkeeper.h"
#include "ScanBase.h"
#include "yarrgui.h"

#include "AllFei4Actions.h"
#include "fei4reghelper.h"
#include "scanstruct.h"
#include "yarrgui.h"

class YarrGui;

namespace Ui {
class CreateScanDialog;
}

class CreateScanDialog : public QDialog {
    Q_OBJECT

public:
    explicit CreateScanDialog(Bookkeeper * bk, QWidget * parent = 0);
    ~CreateScanDialog();

private slots:

    void on_fei4PreSetRegisterButton_clicked();
    void on_fei4PreClearRegisterButton_clicked();

    void on_dcLoopButton_clicked();
    void on_maskLoopButton_clicked();
    void on_triggerLoopButton_clicked();
    void on_horizontalSlider_sliderReleased();
    void on_dataLoopButton_clicked();
    void on_parameterLoopButton_clicked();
    void on_PFbLoopButton_clicked();
    void on_GFbLoopButton_clicked();
    void on_gatherButton_clicked();

    void on_fei4LoopClearButton_clicked();

    void on_fei4PostSetRegisterButton_clicked();
    void on_fei4PostClearRegisterButton_clicked();

    void on_checkOccMap_stateChanged(int arg1);
    void on_checkToTMap_stateChanged(int arg1);
    void on_checkToT2Map_stateChanged(int arg1);
    void on_checkOccAna_stateChanged(int arg1);
    void on_checkNoiseAna_stateChanged(int arg1);
    void on_checkToTAna_stateChanged(int arg1);
    void on_checkScuFit_stateChanged(int arg1);
    void on_checkPixThr_stateChanged(int arg1);

    void on_saveButton_clicked();

private:
    void initDCMode(QComboBox * b);
    void initFeedbackType(QComboBox * b);
    void initFei4RegHelper(QComboBox * b);
    void initMaskStage(QComboBox * b);
    void initVerbose(QComboBox * b);

    Ui::CreateScanDialog * ui;

    CustomScan myScan;
    YarrGui * localParent;
};

Q_DECLARE_METATYPE(DC_MODE)
Q_DECLARE_METATYPE(FeedbackType)
Q_DECLARE_METATYPE(Fei4RegHelper)
Q_DECLARE_METATYPE(MASK_STAGE)

#endif // CREATESCANDIALOG_H
