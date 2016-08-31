#include "createscandialog.h"
#include "ui_createscandialog.h"

CreateScanDialog::CreateScanDialog(Bookkeeper * bk, QWidget * parent) :
    QDialog(parent),
    ui(new Ui::CreateScanDialog),
    myScan(CustomScan(bk))
    {
        ui->setupUi(this);

        this->initFei4RegHelper(ui->fei4PreRegisterBox);

        ui->prescanTable->setColumnCount(2);
        QStringList headerList;
        headerList.append("Fei4 Register");
        headerList.append("Value");
        ui->prescanTable->setHorizontalHeaderLabels(headerList);
        ui->prescanTable->setColumnWidth(0, 220);
        ui->prescanTable->setColumnWidth(1, 220);

        this->initDCMode(ui->dcModeBox);
        this->initVerbose(ui->dcVerboseBox);

        this->initMaskStage(ui->maskStageBox);
        this->initVerbose(ui->maskVerboseBox);
        this->initVerbose(ui->maskScapBox);
        this->initVerbose(ui->maskLcapBox);

        this->initVerbose(ui->triggerVerboseBox);

        this->initVerbose(ui->dataVerboseBox);

        this->initVerbose(ui->parameterVerboseBox);
        this->initFei4RegHelper(ui->parameterRangeRegisterBox);

        this->initVerbose(ui->pixelVerboseBox);
        this->initFeedbackType(ui->pixelFeedbackBox);

        this->initVerbose(ui->globalVerboseBox);
        this->initFei4RegHelper(ui->globalRegisterBox);

        this->initVerbose(ui->gatherVerboseBox);

        ui->loopStructureText->setReadOnly(true);

        this->initFei4RegHelper(ui->fei4PostRegisterBox);

        QMessageBox warningBox(this);
        warningBox.setWindowTitle("WARNING");
        warningBox.setText(
                    "<center>Use with <b>extreme caution! <br>Do not use</b> unless you <b>REALLY</b> know what you are doing! </center>");
        warningBox.exec();

        ui->horizontalSlider->setRange(0, 1999);

        ui->horizontalSlider->setSliderPosition(0);
        ui->triggerTimeBox->setEnabled(false);

        ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

        ui->owningFrame->setMinimumWidth(1200);
        ui->owningFrame->setMinimumHeight(1450);

        this->resize(800, 450);

        localParent = dynamic_cast<YarrGui*>(parent);
        if(localParent == nullptr) {
            std::cerr << "Severe cast error. YarrGui not accessible! \n";
            this->close();
        }

//        std::cout << ui->verticalLayout->minimumSize().width() << std::endl; //DEBUG
//        std::cout << ui->verticalLayout->minimumSize().height() << std::endl; //DEBUG
//        QList<int> tmpList1;
//        tmpList1.append(ui->verticalLayout->minimumSize().width());
//        ui->splitter_2->setSizes();

        QList<int> tmpList;
        tmpList.append(2500);
        tmpList.append(5000);

//        tmpList.append(ui->splitter->width() - ui->verticalLayout_2->minimumSize().width());
//        tmpList.append(ui->verticalLayout_2->minimumSize().width());
//        ui->splitter->setSizes(tmpList);
//        tmpList.clear();

//        tmpList.append(ui->splitter_2->width() - ui->verticalLayout->minimumSize().width() - 200);
//        tmpList.append(ui->verticalLayout->minimumSize().width());
//        ui->splitter_2->setSizes(tmpList);
//        tmpList.clear();

        ui->splitter->setSizes(tmpList);
        ui->splitter_2->setSizes(tmpList);
        ui->splitter_3->setSizes(tmpList);
}

CreateScanDialog::~CreateScanDialog() {
    delete ui;
}

void CreateScanDialog::on_fei4PreSetRegisterButton_clicked() {
    QString insert0 = ui->fei4PreRegisterBox->currentText();
    QString insert1 = ui->fei4PreRegValBox->text();
    int numOfRows = ui->prescanTable->rowCount();

    int index = ui->fei4PreRegisterBox->currentIndex();

    Fei4RegHelper f = ui->fei4PreRegisterBox->itemData(index).value<Fei4RegHelper>();
    uint16_t i = ui->fei4PreRegValBox->value();

    if(std::pow(2, f.getMask()) - 1 < i) {
        std::cout << "ERROR! Value to large to be stored in that register!\n";
        return;
    }

    ui->prescanTable->insertRow(numOfRows);
    ui->prescanTable->setItem(numOfRows, 0,
                                  new QTableWidgetItem(insert0));
    ui->prescanTable->setItem(numOfRows, 1,
                              new QTableWidgetItem(insert1));

    myScan.addPreScanReg(f, i);

    return;
}

void CreateScanDialog::on_fei4PreClearRegisterButton_clicked() {
    ui->prescanTable->clear();
    while(ui->prescanTable->rowCount() > 0) {
        ui->prescanTable->removeRow(0);
    }

    myScan.clearPreScanRegs();

    return;
}

void CreateScanDialog::on_dcLoopButton_clicked() {
    int sLS = myScan.scanLoopsSize();

    std::shared_ptr<Fei4DcLoop> f(new Fei4DcLoop());

    f->setMode(ui->dcModeBox->itemData(ui->dcModeBox->currentIndex()).value<DC_MODE>());
    f->setVerbose(ui->dcVerboseBox->itemData(ui->dcVerboseBox->currentIndex()).value<bool>());

    myScan.addScanLoop(f);

    QString ins = QString::number(sLS + 1) + ". DC Loop\n";
    QString tmp = ui->loopStructureText->toPlainText();
    QString ind = "";
    for(int i = 0; i < sLS; i++) {
        ind.append("    ");
    }

    tmp = tmp + ind + ins;

    ui->loopStructureText->setPlainText(tmp);

    return;
}

void CreateScanDialog::on_maskLoopButton_clicked() {
    int sLS = myScan.scanLoopsSize();

    std::shared_ptr<Fei4MaskLoop> f(new Fei4MaskLoop());

    f->setMaskStage(ui->maskStageBox->itemData(ui->maskStageBox->currentIndex()).value<MASK_STAGE>());
    f->setVerbose(ui->maskVerboseBox->itemData(ui->maskVerboseBox->currentIndex()).value<bool>());
    f->setScap(ui->maskScapBox->itemData(ui->maskScapBox->currentIndex()).value<bool>());
    f->setLcap(ui->maskLcapBox->itemData(ui->maskLcapBox->currentIndex()).value<bool>());

    myScan.addScanLoop(f);

    QString ins = QString::number(sLS + 1) + ". Mask Loop\n";
    QString tmp = ui->loopStructureText->toPlainText();
    QString ind = "";
    for(int i = 0; i < sLS; i++) {
        ind.append("    ");
    }

    tmp = tmp + ind + ins;

    ui->loopStructureText->setPlainText(tmp);

    return;
}

void CreateScanDialog::on_triggerLoopButton_clicked() {
    int sLS = myScan.scanLoopsSize();

    std::shared_ptr<Fei4TriggerLoop> f(new Fei4TriggerLoop());

    f->setVerbose(ui->triggerVerboseBox->itemData(ui->triggerVerboseBox->currentIndex()).value<bool>());
    f->setTrigDelay(ui->triggerDelayBox->value());
    f->setTrigFreq(ui->triggerFrequencyBox->value());
    if(ui->triggerCountBox->isEnabled()) {
        f->setTrigCnt(ui->triggerCountBox->value());
    } else if(ui->triggerTimeBox->isEnabled()) {
        f->setTrigTime(ui->triggerTimeBox->value());
    } else {
        std::cerr << "Severe interface error. Aborting... \n";
        this->close();
    }

    myScan.addScanLoop(f);

    QString ins = QString::number(sLS + 1) + ". Trigger Loop\n";
    QString tmp = ui->loopStructureText->toPlainText();
    QString ind = "";
    for(int i = 0; i < sLS; i++) {
        ind.append("    ");
    }

    tmp = tmp + ind + ins;

    ui->loopStructureText->setPlainText(tmp);

    return;
}

void CreateScanDialog::on_horizontalSlider_sliderReleased() {
    if(ui->horizontalSlider->sliderPosition() < 1000) {
        ui->horizontalSlider->setSliderPosition(0);
        ui->triggerCountBox->setEnabled(true);
        ui->triggerTimeBox->setEnabled(false);
    } else {
        ui->horizontalSlider->setSliderPosition(1999);
        ui->triggerCountBox->setEnabled(false);
        ui->triggerTimeBox->setEnabled(true);
    }
    return;
}

void CreateScanDialog::on_dataLoopButton_clicked() {
    int sLS = myScan.scanLoopsSize();

    std::shared_ptr<StdDataLoop> f(new StdDataLoop());

    f->setVerbose(ui->dataVerboseBox->itemData(ui->dataVerboseBox->currentIndex()).value<bool>());
    f->connect(myScan.getGData());

    myScan.addScanLoop(f);

    QString ins = QString::number(sLS + 1) + ". Data Loop\n";
    QString tmp = ui->loopStructureText->toPlainText();
    QString ind = "";
    for (int i = 0; i < sLS; i++) {
        ind.append("    ");
    }

    tmp = tmp + ind + ins;

    ui->loopStructureText->setPlainText(tmp);

    return;
}

void CreateScanDialog::on_parameterLoopButton_clicked() {
    int sLS = myScan.scanLoopsSize();

    Fei4RegHelper h(ui->parameterRangeRegisterBox->itemData(ui->parameterRangeRegisterBox->currentIndex()).value<Fei4RegHelper>());
    unsigned int p1 = h.getMOffset();
    unsigned int p2 = h.getBOffset();
    unsigned int p3 = h.getMask();
    bool p4 = h.getMsbRight();

    std::shared_ptr<Fei4PLHelper> f(new Fei4PLHelper(p1, p2, p3, p4));

    f->setMin(ui->parameterMinBox->value());
    f->setMax(ui->parameterMaxBox->value());
    f->setStep(ui->parameterStepBox->value());
    f->setVerbose(ui->parameterVerboseBox->itemData(ui->parameterVerboseBox->currentIndex()).value<bool>());

    myScan.addScanLoop(f);

    QString ins = QString::number(sLS + 1) + ". Parameter Loop\n";
    QString tmp = ui->loopStructureText->toPlainText();
    QString ind = "";
    for(int i = 0; i < sLS; i++) {
        ind.append("    ");
    }

    tmp = tmp + ind + ins;

    ui->loopStructureText->setPlainText(tmp);

    return;
}

void CreateScanDialog::on_PFbLoopButton_clicked() {
    int sLS = myScan.scanLoopsSize();

    std::shared_ptr<Fei4PixelFeedback>
        f(new Fei4PixelFeedback(ui->pixelFeedbackBox->itemData(ui->pixelVerboseBox->currentIndex()).value<FeedbackType>()));

    f->setVerbose(ui->pixelVerboseBox->itemData(ui->pixelVerboseBox->currentIndex()).value<bool>());

    myScan.addScanLoop(f);

    QString ins = QString::number(sLS + 1) + ". Pixel Feedback\n";
    QString tmp = ui->loopStructureText->toPlainText();
    QString ind = "";
    for (int i = 0; i < sLS; i++) {
        ind.append("    ");
    }

    tmp = tmp + ind + ins;

    ui->loopStructureText->setPlainText(tmp);

    return;

}

//under construction
void CreateScanDialog::on_GFbLoopButton_clicked() {
    int sLS = myScan.scanLoopsSize();

    std::shared_ptr<Fei4GFHelper>
            f(new Fei4GFHelper(ui->globalRegisterBox->itemData(ui->globalRegisterBox->currentIndex()).value<Fei4RegHelper>()));

    f->setMin(ui->globalMinBox->value());
    f->setMax(ui->globalMaxBox->value());
    f->setStep(ui->globalStepBox->value());
    f->setVerbose(ui->dataVerboseBox->itemData(ui->dataVerboseBox->currentIndex()).value<bool>());

    myScan.addScanLoop(f);

    QString ins = QString::number(sLS + 1) + ". Global Feedback\n";
    QString tmp = ui->loopStructureText->toPlainText();
    QString ind = "";
    for (int i = 0; i < sLS; i++) {
        ind.append("    ");
    }

    tmp = tmp + ind + ins;

    ui->loopStructureText->setPlainText(tmp);

    return;
}

void CreateScanDialog::on_gatherButton_clicked() {
    int sLS = myScan.scanLoopsSize();

    std::shared_ptr<StdDataGatherer> f(new StdDataGatherer());

    f->setVerbose(ui->gatherVerboseBox->itemData(ui->gatherVerboseBox->currentIndex()).value<bool>());

    myScan.addScanLoop(f);

    QString ins = QString::number(sLS + 1) + ". Data Gather\n";
    QString tmp = ui->loopStructureText->toPlainText();
    QString ind = "";
    for (int i = 0; i < sLS; i++) {
        ind.append("    ");
    }

    tmp = tmp + ind + ins;

    ui->loopStructureText->setPlainText(tmp);

    return;
}

void CreateScanDialog::on_fei4LoopClearButton_clicked() {
    ui->loopStructureText->clear();

    myScan.clearScanLoops();

    return;
}

void CreateScanDialog::on_fei4PostSetRegisterButton_clicked() {
    QString insert0 = ui->fei4PostRegisterBox->currentText();
    QString insert1 = ui->fei4PostRegValBox->text();
    int numOfRows = ui->postscanTable->rowCount();

    int index = ui->fei4PostRegisterBox->currentIndex();

    Fei4RegHelper f = ui->fei4PostRegisterBox->itemData(index).value<Fei4RegHelper>();
    uint16_t i = ui->fei4PostRegValBox->value();

    ui->postscanTable->insertRow(numOfRows);
    ui->postscanTable->setItem(numOfRows, 0,
                                  new QTableWidgetItem(insert0));
    ui->postscanTable->setItem(numOfRows, 1,
                              new QTableWidgetItem(insert1));

    myScan.addPostScanReg(f, i);

    return;
}

void CreateScanDialog::on_fei4PostClearRegisterButton_clicked() {
    ui->postscanTable->clear();
    while(ui->postscanTable->rowCount() > 0) {
        ui->postscanTable->removeRow(0);
    }

    myScan.clearPostScanRegs();

    return;
}

void CreateScanDialog::initDCMode(QComboBox * b) {
    QString s;
    QVariant v;

    s = "SINGLE";
    v.setValue(SINGLE_DC);
    b->addItem(s, v);

    s = "QUAD";
    v.setValue(QUAD_DC);
    b->addItem(s, v);

    s = "OCTA";
    v.setValue(OCTA_DC);
    b->addItem(s, v);

    s = "ALL";
    v.setValue(ALL_DC);
    b->addItem(s, v);

    return;
}

void CreateScanDialog::initFeedbackType(QComboBox * b) {
    QString s;
    QVariant v;

    s = "TDAC_FB";
    v.setValue(TDAC_FB);
    b->addItem(s, v);

    s = "FDAC_FB";
    v.setValue(FDAC_FB);
    b->addItem(s, v);

    return;
}

void CreateScanDialog::initFei4RegHelper(QComboBox * b) {
    QString s;
    QVariant v;

    //1
    s = "SME";
    v.setValue(Fei4RegHelper(1,8,1,false));
    b->addItem(s, v);

    s = "EventLimit";
    v.setValue(Fei4RegHelper(1,0,8,true));
    b->addItem(s, v);

    //2
    s = "Trig_Count";
    v.setValue(Fei4RegHelper(2,12,4,false));
    b->addItem(s, v);

    s = "Conf_AddrEnable";
    v.setValue(Fei4RegHelper(2,11,1,false));
    b->addItem(s, v);

    //3
    s = "ErrorMask_0";
    v.setValue(Fei4RegHelper(3,0,16,false));
    b->addItem(s, v);

    //4
    s = "ErrorMask_1";
    v.setValue(Fei4RegHelper(4,0,16,false));
    b->addItem(s, v);

    //5
    s = "PrmpVbp_R";
    v.setValue(Fei4RegHelper(5,8,8,true));
    b->addItem(s, v);

    s = "BufVgOpAmp";
    v.setValue(Fei4RegHelper(5,0,8,true));
    b->addItem(s, v);

    s = "GADCVref";
    v.setValue(Fei4RegHelper(5,0,8,true));
    b->addItem(s, v);

    //6
    s = "PrmpVbp";
    v.setValue(Fei4RegHelper(6,0,8,true));
    b->addItem(s, v);

    //7
    s = "TDACVbp";
    v.setValue(Fei4RegHelper(7,8,8,true));
    b->addItem(s, v);

    s = "DisVbn";
    v.setValue(Fei4RegHelper(7,0,8,true));
    b->addItem(s, v);

    //8
    s = "Amp2Vbn";
    v.setValue(Fei4RegHelper(8,8,8,true));
    b->addItem(s, v);

    s = "Amp2VbpFol";
    v.setValue(Fei4RegHelper(8,0,8,true));
    b->addItem(s, v);

    //9
    s = "Amp2Vbp";
    v.setValue(Fei4RegHelper(9,0,8,true));
    b->addItem(s, v);

    //10
    s = "FDACVbn";
    v.setValue(Fei4RegHelper(10,8,8,true));
    b->addItem(s, v);

    s = "Amp2Vbpff";
    v.setValue(Fei4RegHelper(10,0,8,true));
    b->addItem(s, v);

    //11
    s = "PrmpVbnFol";
    v.setValue(Fei4RegHelper(11,8,8,true));
    b->addItem(s, v);

    s = "PrmpVbp_L";
    v.setValue(Fei4RegHelper(11,0,8,true));
    b->addItem(s, v);

    //12
    s = "PrmpVbpf";
    v.setValue(Fei4RegHelper(12,8,8,true));
    b->addItem(s, v);

    s = "PrmpVbnLCC";
    v.setValue(Fei4RegHelper(12,0,8,true));
    b->addItem(s, v);

    //13
    s = "S1";
    v.setValue(Fei4RegHelper(13,15,1,false));
    b->addItem(s, v);

    s = "S0";
    v.setValue(Fei4RegHelper(13,14,1,false));
    b->addItem(s, v);

    s = "Pixel_latch_strobe";
    v.setValue(Fei4RegHelper(13,1,13,true));
    b->addItem(s, v);

    //14
    s = "LVDSDrvIref";
    v.setValue(Fei4RegHelper(14,8,8,true));
    b->addItem(s, v);

    s = "GADCCompBias";
    v.setValue(Fei4RegHelper(14,0,8,true));
    b->addItem(s, v);

    //15
    s = "PllIbias";
    v.setValue(Fei4RegHelper(15,8,8,true));
    b->addItem(s, v);

    s = "LVDSDrvVos";
    v.setValue(Fei4RegHelper(15,0,8,true));
    b->addItem(s, v);

    //16
    s = "TempSensIbias";
    v.setValue(Fei4RegHelper(16,8,8,true));
    b->addItem(s, v);

    s = "PllIcp";
    v.setValue(Fei4RegHelper(16,0,8,true));
    b->addItem(s, v);

    //17
    s = "PlsrIDACRamp";
    v.setValue(Fei4RegHelper(17,0,8,true));
    b->addItem(s, v);

    //18
    s = "VrefDigTune";
    v.setValue(Fei4RegHelper(18,8,8,true));
    b->addItem(s, v);

    s = "PlsrVgOpAmp";
    v.setValue(Fei4RegHelper(18,0,8,true));
    b->addItem(s, v);

    //19
    s = "PlsrDACbias";
    v.setValue(Fei4RegHelper(19,8,8,true));
    b->addItem(s, v);

    s = "VrefAnTune";
    v.setValue(Fei4RegHelper(19,0,8,true));
    b->addItem(s, v);

    //20
    s = "Vthin_Coarse";
    v.setValue(Fei4RegHelper(20,8,8,true));
    b->addItem(s, v);

    s = "Vthin_Fine";
    v.setValue(Fei4RegHelper(20,0,8,true));
    b->addItem(s, v);

    //21
    s = "HitLD";
    v.setValue(Fei4RegHelper(21,12,1,false));
    b->addItem(s, v);

    s = "DJO";
    v.setValue(Fei4RegHelper(21,11,1,false));
    b->addItem(s, v);

    s = "DigHitIn_Sel";
    v.setValue(Fei4RegHelper(21,10,1,false));
    b->addItem(s, v);

    s = "PlsrDAC";
    v.setValue(Fei4RegHelper(21,0,10,true));
    b->addItem(s, v);

    //22
    s = "Colpr_Mode";
    v.setValue(Fei4RegHelper(22,8,2,true));
    b->addItem(s, v);

    s = "Colpr_Addr";
    v.setValue(Fei4RegHelper(22,2,6,true));
    b->addItem(s, v);

    //23
    s = "DisableColCnfg0";
    v.setValue(Fei4RegHelper(23,0,16,false));
    b->addItem(s, v);

    //24
    s = "DisableColCnfg1";
    v.setValue(Fei4RegHelper(24,0,16,false));
    b->addItem(s, v);

    //25
    s = "Trig_Lat";
    v.setValue(Fei4RegHelper(25,8,8,false));
    b->addItem(s, v);

    s = "DisableColCnfg2";
    v.setValue(Fei4RegHelper(25,0,8,false));
    b->addItem(s, v);

    //26
    s = "CMDcnt12";
    v.setValue(Fei4RegHelper(26,3,13,false));
    b->addItem(s, v);

    s = "CalPulseWidth";
    v.setValue(Fei4RegHelper(26,3,8,false));
    b->addItem(s, v);

    s = "CalPulseDelay";
    v.setValue(Fei4RegHelper(26,11,5,false));
    b->addItem(s, v);

    s = "StopModeConfig";
    v.setValue(Fei4RegHelper(26,2,1,false));
    b->addItem(s, v);

    s = "HitDiscCnfg";
    v.setValue(Fei4RegHelper(26,0,2,false));
    b->addItem(s, v);

    //27
    s = "PLL_Enable";
    v.setValue(Fei4RegHelper(27,15,1,false));
    b->addItem(s, v);

    s = "EFS";
    v.setValue(Fei4RegHelper(27,14,1,false));
    b->addItem(s, v);

    s = "StopClkPulse";
    v.setValue(Fei4RegHelper(27,13,1,false));
    b->addItem(s, v);

    s = "ReadErrorReq";
    v.setValue(Fei4RegHelper(27,12,1,false));
    b->addItem(s, v);

    s = "GADC_En";
    v.setValue(Fei4RegHelper(27,10,1,false));
    b->addItem(s, v);

    s = "SRRead";
    v.setValue(Fei4RegHelper(27,9,1,false));
    b->addItem(s, v);

    s = "HitOr";
    v.setValue(Fei4RegHelper(27,5,1,false));
    b->addItem(s, v);

    s = "CalEn";
    v.setValue(Fei4RegHelper(27,4,1,false));
    b->addItem(s, v);

    s = "SRClr";
    v.setValue(Fei4RegHelper(27,3,1,false));
    b->addItem(s, v);

    s = "Latch_Enable";
    v.setValue(Fei4RegHelper(27,2,1,false));
    b->addItem(s, v);

    s = "SR_Clock";
    v.setValue(Fei4RegHelper(27,1,1,false));
    b->addItem(s, v);

    s = "M13";
    v.setValue(Fei4RegHelper(27,0,1,false));
    b->addItem(s, v);

    //28
    s = "LVDSDrvSet06";
    v.setValue(Fei4RegHelper(28,15,1,false));
    b->addItem(s, v);

    s = "EN_40M";
    v.setValue(Fei4RegHelper(28,9,1,false));
    b->addItem(s, v);

    s = "EN_80M";
    v.setValue(Fei4RegHelper(28,8,1,false));
    b->addItem(s, v);

    s = "CLK1_S0";
    v.setValue(Fei4RegHelper(28,7,1,false));
    b->addItem(s, v);

    s = "CLK1_S1";
    v.setValue(Fei4RegHelper(28,6,1,false));
    b->addItem(s, v);

    s = "CLK1_S2";
    v.setValue(Fei4RegHelper(28,5,1,false));
    b->addItem(s, v);

    s = "CLK0_S0";
    v.setValue(Fei4RegHelper(28,4,1,false));
    b->addItem(s, v);

    s = "CLK0_S1";
    v.setValue(Fei4RegHelper(28,3,1,false));
    b->addItem(s, v);

    s = "CLK0_S2";
    v.setValue(Fei4RegHelper(28,2,1,false));
    b->addItem(s, v);

    s = "EN_160";
    v.setValue(Fei4RegHelper(28,1,1,false));
    b->addItem(s, v);

    s = "EN_320";
    v.setValue(Fei4RegHelper(28,0,1,false));
    b->addItem(s, v);

    //29
    s = "No8b10b";
    v.setValue(Fei4RegHelper(29,13,1,false));
    b->addItem(s, v);

    s = "Clk2Out";
    v.setValue(Fei4RegHelper(29,12,1,false));
    b->addItem(s, v);

    s = "EmptyRecordCnfg";
    v.setValue(Fei4RegHelper(29,4,8,false));
    b->addItem(s, v);

    s = "LVDSDrvEn";
    v.setValue(Fei4RegHelper(29,2,1,false));
    b->addItem(s, v);

    s = "LVDSDrvSet30";
    v.setValue(Fei4RegHelper(29,1,1,false));
    b->addItem(s, v);

    s = "LVDSDrvSet12";
    v.setValue(Fei4RegHelper(29,0,1,false));
    b->addItem(s, v);

    //30
    s = "TmpSensDiodeSel";
    v.setValue(Fei4RegHelper(30,14,2,false));
    b->addItem(s, v);

    s = "TmpSensDisable";
    v.setValue(Fei4RegHelper(30,13,1,false));
    b->addItem(s, v);

    s = "IleakRange";
    v.setValue(Fei4RegHelper(30,12,1,false));
    b->addItem(s, v);

    //31
    s = "PlsrRiseUpTau";
    v.setValue(Fei4RegHelper(31,13,3,false));
    b->addItem(s, v);

    s = "PlsrPwr";
    v.setValue(Fei4RegHelper(31,12,1,false));
    b->addItem(s, v);

    s = "PlsrDelay";
    v.setValue(Fei4RegHelper(31,6,6,true));
    b->addItem(s, v);

    s = "ExtDigCalSW";
    v.setValue(Fei4RegHelper(31,5,1,false));
    b->addItem(s, v);

    s = "ExtAnaCalSW";
    v.setValue(Fei4RegHelper(31,4,1,false));
    b->addItem(s, v);

    s = "GADCSel";
    v.setValue(Fei4RegHelper(31,0,3,false));
    b->addItem(s, v);

    //32
    s = "SELB0";
    v.setValue(Fei4RegHelper(32,0,16,false));
    b->addItem(s, v);

    //33
    s = "SELB1";
    v.setValue(Fei4RegHelper(33,0,16,false));
    b->addItem(s, v);

    //34
    s = "SELB2";
    v.setValue(Fei4RegHelper(34,8,8,false));
    b->addItem(s, v);

    s = "PrmpVbpMsbEn";
    v.setValue(Fei4RegHelper(34,4,1,false));
    b->addItem(s, v);

    //35
    s = "EFUSE";
    v.setValue(Fei4RegHelper(35,0,16,false));
    b->addItem(s, v);

    return;
}

void CreateScanDialog::initMaskStage(QComboBox * b) {
    QString s;
    QVariant v;

    s = "MASK_1";
    v.setValue(MASK_1);
    b->addItem(s, v);

    s = "MASK_2";
    v.setValue(MASK_2);
    b->addItem(s, v);

    s = "MASK_4";
    v.setValue(MASK_4);
    b->addItem(s, v);

    s = "MASK_8";
    v.setValue(MASK_8);
    b->addItem(s, v);

    s = "MASK_16";
    v.setValue(MASK_16);
    b->addItem(s, v);

    s = "MASK_32";
    v.setValue(MASK_32);
    b->addItem(s, v);

    s = "MASK_NONE";
    v.setValue(MASK_NONE);
    b->addItem(s, v);

    return;
}

void CreateScanDialog::initVerbose(QComboBox * b) {
    QString s;
    QVariant v;

    s = "TRUE";
    v.setValue(true);
    b->addItem(s, v);

    s = "FALSE";
    v.setValue(false);
    b->addItem(s, v);

    return;
}

void CreateScanDialog::on_checkOccMap_stateChanged(int arg1) {
    if(arg1 == Qt::Unchecked) {
        myScan.bA.at(OCC_MAP) = false;            //change to enum or enum class some time, this is not safe
    } else if(arg1 == Qt::Checked) {
        myScan.bA.at(OCC_MAP) = true;
    }

    return;
}

void CreateScanDialog::on_checkToTMap_stateChanged(int arg1) {
    if(arg1 == Qt::Unchecked) {
        myScan.bA.at(TOT_MAP) = false;
    } else if(arg1 == Qt::Checked) {
        myScan.bA.at(TOT_MAP) = true;
    }

    return;
}

void CreateScanDialog::on_checkToT2Map_stateChanged(int arg1) {
    if(arg1 == Qt::Unchecked) {
        myScan.bA.at(TOT_2_MAP) = false;
    } else if(arg1 == Qt::Checked) {
        myScan.bA.at(TOT_2_MAP) = true;
    }

    return;
}

void CreateScanDialog::on_checkOccAna_stateChanged(int arg1) {
    if(arg1 == Qt::Unchecked) {
        myScan.bA.at(OCC_ANA) = false;
    } else if(arg1 == Qt::Checked) {
        myScan.bA.at(OCC_ANA) = true;
    }

    return;
}

void CreateScanDialog::on_checkNoiseAna_stateChanged(int arg1) {
    if(arg1 == Qt::Unchecked) {
        myScan.bA.at(NOISE_ANA) = false;
    } else if(arg1 == Qt::Checked) {
        myScan.bA.at(NOISE_ANA) = true;
    }

    return;
}

void CreateScanDialog::on_checkToTAna_stateChanged(int arg1) {
    if(arg1 == Qt::Unchecked) {
        myScan.bA.at(TOT_ANA) = false;
    } else if(arg1 == Qt::Checked) {
        myScan.bA.at(TOT_ANA) = true;
    }

    return;
}

void CreateScanDialog::on_checkScuFit_stateChanged(int arg1) {
    if(arg1 == Qt::Unchecked) {
        myScan.bA.at(S_CU_FIT) = false;
    } else if(arg1 == Qt::Checked) {
        myScan.bA.at(S_CU_FIT) = true;
    }

    return;
}

void CreateScanDialog::on_checkPixThr_stateChanged(int arg1) {
    if(arg1 == Qt::Unchecked) {
        myScan.bA.at(PIX_THR) = false;
    } else if(arg1 == Qt::Checked) {
        myScan.bA.at(PIX_THR) = true;
    }

    return;
}

void CreateScanDialog::on_saveButton_clicked() {
    localParent->setCustomScan(myScan);
    this->close();

    return;
}

