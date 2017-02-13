#include "editcfgdialog.h"
#include "ui_editcfgdialog.h"

EditCfgDialog::EditCfgDialog(Fei4 * f, QString cfgFNJ_param, QWidget * parent) :
    QDialog(parent),
    ui(new Ui::EditCfgDialog),
    fE(f),
    cfgFNJ(cfgFNJ_param),
    eM(editMode::ONE),
    eN(1),
    pD(nullptr)
{
    ui->setupUi(this);

    this->fE->toFileJson(this->j);

    this->ui->cfgTable->setRowCount(336);
    this->ui->cfgTable->setColumnCount(80);
    for(unsigned int i = 0; i < 336; i += 1){
        this->ui->cfgTable->setRowHeight(i, 10);
    }
    for(unsigned int i = 0; i < 80; i += 1){
        this->ui->cfgTable->setColumnWidth(i, 15);
    }
    this->ui->tableLegend->setMaximumHeight(52);
    this->ui->cfgTable->horizontalHeader()->hide();
    this->ui->tableLegend->horizontalHeader()->hide();
    this->ui->cfgTable->verticalHeader()->hide();
    this->ui->tableLegend->verticalHeader()->hide();
    QFont* zeroFont = new QFont;
    zeroFont->setPixelSize(1);
    this->ui->cfgTable->setFont(*zeroFont);
    this->ui->cfgTable->setMouseTracking(true);
    QObject::connect(this->ui->cfgTable, SIGNAL(cellClicked(int,int)), this, SLOT(clickHandler(int,int)));
    QObject::connect(this->ui->cfgTable, SIGNAL(cellEntered(int,int)), this, SLOT(enterHandler(int,int)));
    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][this->ui->EnableRadio->text().toStdString().c_str()][k];
            this->ui->cfgTable->setItem(i, k, new QTableWidgetItem(QString::number(l)));
            this->ui->cfgTable->item(i, k)->setBackgroundColor(*(this->fetchColor(l, 1)));
            this->ui->cfgTable->item(i, k)->setFlags(0x0);
        }
    }

    this->setWindowState(Qt::WindowMaximized);
    this->initLegend();
    this->initGRCB(this->ui->GRCombo);

    this->ui->GRTable->setRowCount(35);
    this->ui->GRTable->setColumnCount(16);

    for(unsigned int i = 0; i<35; i+=1){
        this->ui->GRTable->setRowHeight(i, 16);
    }
    for(unsigned int i = 0; i<16; i+=1){
        this->ui->GRTable->setColumnWidth(i, 64);
    }
    this->ui->GRTable->setFont(*zeroFont);
    this->ui->GRTable->setMouseTracking(true);
    QObject::connect(this->ui->GRTable, SIGNAL(cellClicked(int,int)), this, SLOT(clickHandlerGR(int,int)));
    for(unsigned int row = 0; row<35; row+=1){
        for(unsigned int col = 0; col<16; col+=1){
            this->ui->GRTable->setItem(row, col, new QTableWidgetItem("0"));
            this->ui->GRTable->item(row, col)->setFlags(0x0);
        }
    }
    QObject::connect(this->ui->GRCombo, SIGNAL(currentIndexChanged(QString)), this, SLOT(normalizeHandler(QString)));
    this->grey.setRgb(220, 220, 220);
    this->darkyellow.setRgb(200, 200, 50);
    this->lightyellow.setRgb(255, 255, 128);
    this->darkgreen.setRgb(0, 179, 0);
    this->lightgreen.setRgb(0, 220, 0);
    this->normalizeGRColors();
//    this->ui->GRTable->item(0, 0)->setBackgroundColor(*grey);
//    for(int row = 0; row<35; row+=1){
//        for(int col = 0; col<16; col+=1){
//            this->ui->GRTable->item(row, col)->setBackgroundColor(*grey);
//        }
//    }
    QObject::connect(this->ui->GRSpin, SIGNAL(valueChanged(int)), this, SLOT(updateHandlerGR(int)));

    this->ui->chipIdSpin->setValue((int)this->j["FE-I4B"]["Parameter"]["chipId"]);
    this->ui->lCapDSpin->setValue((double)this->j["FE-I4B"]["Parameter"]["lCap"]);
    this->ui->sCapDSpin->setValue((double)this->j["FE-I4B"]["Parameter"]["sCap"]);
    this->ui->vcalOffsetDSpin->setValue((double)this->j["FE-I4B"]["Parameter"]["vcalOffset"]);
    this->ui->vcalSlopeDSpin->setValue((double)this->j["FE-I4B"]["Parameter"]["vcalSlope"]);
    std::string tmpStr = this->j["FE-I4B"]["name"];
    this->ui->chipNameLine->setText(QString::fromStdString(tmpStr));
    this->ui->rxChannelSpin->setValue((int)this->j["FE-I4B"]["txChannel"]);
    this->ui->rxChannelSpin->setValue((int)this->j["FE-I4B"]["rxChannel"]);

    this->ui->GRCombo->setCurrentIndex(1);
}

EditCfgDialog::~EditCfgDialog(){
    delete ui;
}

//############### GLOBAL ###############

void EditCfgDialog::initGRCB(QComboBox* b){
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

//############### PIXEL ###############

void EditCfgDialog::on_saveButton_clicked(){
    try{
        this->fE->fromFileJson(this->j);
    }
    catch(std::domain_error){
        std::cerr << "ERROR - Malformed argument - aborting" << std::endl;
        return;
    }
    QFile oF(this->cfgFNJ);
    oF.open(QIODevice::WriteOnly);
    QString qS;
    qS = QString::fromStdString(this->j.dump(4));
    QTextStream oS(&oF);
    oS << qS;
    oF.close();

    return;
}

void EditCfgDialog::on_applyButton_clicked(){
    try{
        this->fE->fromFileJson(this->j);
    }
    catch(std::domain_error){
        std::cout << "ERROR - Malformed config - aborting" << std::endl;
        return;
    }

    return;
}

void EditCfgDialog::on_saveAsButton_clicked(){
    this->releaseKeyboard();
    QString filename = QFileDialog::getSaveFileName(this, tr("Select JSON config file"), "./util/", tr("JSON Config File(*.js *.json)"));
    QFile oF(filename);
    oF.open(QIODevice::WriteOnly);
    QString qS;
    qS = QString::fromStdString(this->j.dump(4));
    QTextStream oS(&oF);
    oS << qS;
    oF.close();
    this->grabKeyboard();

    return;
}

//######################### RADIOS #########################

void EditCfgDialog::on_EnableRadio_clicked(){
    if(!ui->EnableRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k<80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->EnableRadio->text().toStdString().c_str()][k];
            ui->cfgTable->item(i, k)->setText(QString::number(l));
            ui->cfgTable->item(i, k)->setBackgroundColor(*(this->fetchColor(l, this->getMaxVal().second)));
        }
    }

    this->initLegend();

    return;
}

void EditCfgDialog::on_TDACRadio_clicked(){
    if(!ui->TDACRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->TDACRadio->text().toStdString().c_str()][k];
            ui->cfgTable->item(i, k)->setText(QString::number(l));
            ui->cfgTable->item(i, k)->setBackgroundColor(*(this->fetchColor(l, this->getMaxVal().second)));
        }
    }

    this->initLegend();

    return;
}

void EditCfgDialog::on_LCapRadio_clicked(){
    if(!ui->LCapRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->LCapRadio->text().toStdString().c_str()][k];
            ui->cfgTable->item(i, k)->setText(QString::number(l));
            ui->cfgTable->item(i, k)->setBackgroundColor(*(this->fetchColor(l, this->getMaxVal().second)));
        }
    }

    this->initLegend();

    return;
}

void EditCfgDialog::on_SCapRadio_clicked(){
    if(!ui->SCapRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->SCapRadio->text().toStdString().c_str()][k];
            ui->cfgTable->item(i, k)->setText(QString::number(l));
            ui->cfgTable->item(i, k)->setBackgroundColor(*(this->fetchColor(l, this->getMaxVal().second)));
        }
    }

    this->initLegend();

    return;
}

void EditCfgDialog::on_HitbusRadio_clicked(){
    if(!ui->HitbusRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->HitbusRadio->text().toStdString().c_str()][k];
            ui->cfgTable->item(i, k)->setText(QString::number(l));
            ui->cfgTable->item(i, k)->setBackgroundColor(*(this->fetchColor(l, this->getMaxVal().second)));
        }
    }

    this->initLegend();

    return;
}

void EditCfgDialog::on_FDACRadio_clicked(){
    if(!ui->FDACRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->FDACRadio->text().toStdString().c_str()][k];
            ui->cfgTable->item(i, k)->setText(QString::number(l));
            ui->cfgTable->item(i, k)->setBackgroundColor(*(this->fetchColor(l, this->getMaxVal().second)));
        }
    }

    this->initLegend();

    return;
}

//######################### UTILS #########################

QColor* EditCfgDialog::fetchColor(unsigned int v, unsigned int m){
    int r = 1530 * v / (m+1);
    std::array<int, 3> myRgb;
    myRgb.at(0) = 255;
    myRgb.at(1) = 0;
    myRgb.at(2) = 0;

    myRgb.at(1) += (r>255 ? 255 : r);
    r -= (r>255 ? 255 : r);
    myRgb.at(0) -= (r>255 ? 255 : r);
    r -= (r>255 ? 255 : r);
    myRgb.at(2) += (r>255 ? 255 : r);
    r -= (r>255 ? 255 : r);
    myRgb.at(1) -= (r>255 ? 255 : r);
    r -= (r>255 ? 255 : r);
    myRgb.at(0) += (r>255 ? 255 : r);
    r -= (r>255 ? 255 : r);
    myRgb.at(2) -= (r>255 ? 255 : r);
    r -= (r>255 ? 255 : r);

    if(r != 0){
        std::cerr << "QColor RGB calculation went wrong" << std::endl;
    }

    return new QColor(myRgb.at(0), myRgb.at(1), myRgb.at(2));
}

std::pair<std::string, unsigned int> EditCfgDialog::getMaxVal(){
    unsigned int max_val = 0;
    std::string tmpStdStr = "";
    if(ui->EnableRadio->isChecked()){
        max_val = 1;
        tmpStdStr = ui->EnableRadio->text().toStdString();
    }else if(ui->TDACRadio->isChecked()){
        max_val = 31;
        tmpStdStr = ui->TDACRadio->text().toStdString();
    }else if(ui->LCapRadio->isChecked()){
        max_val = 1;
        tmpStdStr = ui->LCapRadio->text().toStdString();
    }else if(ui->SCapRadio->isChecked()){
        max_val = 1;
        tmpStdStr = ui->SCapRadio->text().toStdString();
    }else if(ui->HitbusRadio->isChecked()){
        max_val = 1;
        tmpStdStr = ui->HitbusRadio->text().toStdString();
    }else if(ui->FDACRadio->isChecked()){
        max_val = 15;
        tmpStdStr = ui->FDACRadio->text().toStdString();
    }else{
        std::cerr << "No valid radio button checked. Aborting... " << std::endl;
        this->close();
    }
    return std::pair<std::string, unsigned int>(tmpStdStr, max_val);
}

void EditCfgDialog::clickHandler(int rowCl, int colCl){
    std::pair<std::string, unsigned int> maxVal = this->getMaxVal();
    switch(this->eM){
        case editMode::ONE:{
            int i = ui->cfgTable->item(rowCl, colCl)->text().toInt();
            i += this->eN;
            while(i>(int)maxVal.second){
                i-=(maxVal.second+1);
            }
            while(i<0){
                i+=(maxVal.second+1);
            }
            unsigned int u = (unsigned int)i;
            ui->cfgTable->item(rowCl, colCl)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
            ui->cfgTable->item(rowCl, colCl)->setText(QString::number(u));
//            ui->cfgTable->item(rowCl, colCl)->setToolTip(QString::number(u));
            this->j["FE-I4B"]["PixelConfig"][rowCl][maxVal.first.c_str()][colCl] = u;
            break;
        }
        case editMode::ROW:{
            for(unsigned int k = 0; k < 80; k+=1){
                int i = ui->cfgTable->item(rowCl, k)->text().toInt();
                i += this->eN;
                while(i>(int)maxVal.second){
                    i-=(maxVal.second+1);
                }
                while(i<0){
                    i+=(maxVal.second+1);
                }
                unsigned int u = (unsigned int)i;
                ui->cfgTable->item(rowCl, k)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
                ui->cfgTable->item(rowCl, k)->setText(QString::number(u));
//                ui->cfgTable->item(rowCl, k)->setToolTip(QString::number(u));
                this->j["FE-I4B"]["PixelConfig"][rowCl][maxVal.first.c_str()][k] = u;
            }
            break;
        }
        case editMode::COL:{
            for(unsigned int k = 0; k < 336; k+=1){
                int i = ui->cfgTable->item(k, colCl)->text().toInt();
                i += this->eN;
                while(i>(int)maxVal.second){
                    i-=(maxVal.second+1);
                }
                while(i<0){
                    i+=(maxVal.second+1);
                }
                unsigned int u = (unsigned int)i;
                ui->cfgTable->item(k, colCl)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
                ui->cfgTable->item(k, colCl)->setText(QString::number(u));
//                ui->cfgTable->item(k, colCl)->setToolTip(QString::number(u));
                this->j["FE-I4B"]["PixelConfig"][k][maxVal.first.c_str()][colCl] = u;
            }
            break;
        }
        case editMode::ALL:{
            for(unsigned int k = 0; k<336; k+=1){
                for(unsigned int l = 0; l<80; l+=1){
                    int i = ui->cfgTable->item(k, l)->text().toInt();
                    i += this->eN;
                    while(i>(int)maxVal.second){
                        i-=(maxVal.second+1);
                    }
                    while(i<0){
                        i+=(maxVal.second+1);
                    }
                    unsigned int u = (unsigned int)i;
                    ui->cfgTable->item(k, l)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
                    ui->cfgTable->item(k, l)->setText(QString::number(u));
//                    ui->cfgTable->item(k, l)->setToolTip(QString::number(u));
                    this->j["FE-I4B"]["PixelConfig"][k][maxVal.first.c_str()][l] = u;
                }
            }
            break;
        }
        case editMode::SONE:{
            int i = this->eN;
            if(i<0 || i>maxVal.second){
                std::cerr << "Value exceeds allowed range. Aborting..." << std::endl;
                break;
            }
            unsigned int u = (unsigned int)i;
            ui->cfgTable->item(rowCl, colCl)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
            ui->cfgTable->item(rowCl, colCl)->setText(QString::number(u));
//            ui->cfgTable->item(rowCl, colCl)->setToolTip(QString::number(u));
            this->j["FE-I4B"]["PixelConfig"][rowCl][maxVal.first.c_str()][colCl] = u;
            break;
        }
        case editMode::SROW:{
            int i = this->eN;
            if(i<0 || i>maxVal.second){
                std::cerr << "Value exceeds allowed range. Aborting..." << std::endl;
                break;
            }
            for(unsigned int k = 0; k < 80; k+=1){
                unsigned int u = (unsigned int)i;
                ui->cfgTable->item(rowCl, k)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
                ui->cfgTable->item(rowCl, k)->setText(QString::number(u));
//                ui->cfgTable->item(rowCl, k)->setToolTip(QString::number(u));
                this->j["FE-I4B"]["PixelConfig"][rowCl][maxVal.first.c_str()][k] = u;
            }
            break;
        }
        case editMode::SCOL:{
            int i = this->eN;
            if(i<0 || i>maxVal.second){
                std::cerr << "Value exceeds allowed range. Aborting..." << std::endl;
                break;
            }
            for(unsigned int k = 0; k < 336; k+=1){
                unsigned int u = (unsigned int)i;
                ui->cfgTable->item(k, colCl)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
                ui->cfgTable->item(k, colCl)->setText(QString::number(u));
//                ui->cfgTable->item(k, colCl)->setToolTip(QString::number(u));
                this->j["FE-I4B"]["PixelConfig"][k][maxVal.first.c_str()][colCl] = u;
            }
            break;
        }
        case editMode::SALL:{
            int i = this->eN;
            if(i<0 || i>maxVal.second){
                std::cerr << "Value exceeds allowed range. Aborting..." << std::endl;
                break;
            }
            for(unsigned int k = 0; k<336; k+=1){
                for(unsigned int l = 0; l<80; l+=1){
                    unsigned int u = (unsigned int)i;
                    ui->cfgTable->item(k, l)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
                    ui->cfgTable->item(k, l)->setText(QString::number(u));
//                    ui->cfgTable->item(k, l)->setToolTip(QString::number(u));
                    this->j["FE-I4B"]["PixelConfig"][k][maxVal.first.c_str()][l] = u;
                }
            }
            break;
        }
    }

    return;
}

void EditCfgDialog::enterHandler(int rowEn, int colEn){
    QString tmpQStr = "(" + QString::number(rowEn) + ", " + QString::number(colEn) + ") : "
                          + ui->cfgTable->item(rowEn, colEn)->text();

    ui->linePosInfo->setText(tmpQStr);
}

void EditCfgDialog::initLegend(){
    ui->tableLegend->clear();
    unsigned int numCols = this->getMaxVal().second + 1;
    ui->tableLegend->setRowCount(2);
    ui->tableLegend->setRowHeight(0, 25);
    ui->tableLegend->setRowHeight(1, 25);
    ui->tableLegend->setColumnCount(numCols);
    for(unsigned int i = 0; i < numCols; i+=1){
        ui->tableLegend->setColumnWidth(i, ui->tableLegend->width() / numCols - 1);
        ui->tableLegend->setItem(0, i, new QTableWidgetItem(QString::number(i)));
        ui->tableLegend->setItem(1, i, new QTableWidgetItem);
        ui->tableLegend->item(1, i)->setBackground(*(this->fetchColor(i, numCols-1)));
    }
}

void EditCfgDialog::on_zoomInButton_clicked(){
    int curHe = ui->cfgTable->rowHeight(0);
    int curWi = ui->cfgTable->columnWidth(0);
    for(unsigned int i = 0; i < 336; i += 1){
        ui->cfgTable->setRowHeight(i, curHe + 2);
    }
    for(unsigned int i = 0; i < 80; i += 1){
        ui->cfgTable->setColumnWidth(i, curWi + 3);
    }
}

void EditCfgDialog::on_zoomOutButton_clicked(){
    int curHe = ui->cfgTable->rowHeight(0);
    int curWi = ui->cfgTable->columnWidth(0);
    if(curHe == 2){
        return;
    }
    for(unsigned int i = 0; i < 336; i += 1){
        ui->cfgTable->setRowHeight(i, curHe - 2);
    }
    for(unsigned int i = 0; i < 80; i += 1){
        ui->cfgTable->setColumnWidth(i, curWi - 3);
    }

    return;
}

void EditCfgDialog::keyPressEvent(QKeyEvent* event){
    if(this->ui->cfgTab->currentIndex() == 2){
        if(event->key() == Qt::Key_Control){
            this->grabKeyboard();
            this->pD = new PointerDialog(this);
            this->pD->exec();
        }else{
            event->ignore();
        }
    }else{
        event->ignore();
    }
}

void EditCfgDialog::keyReleaseEvent(QKeyEvent* event){
    if(this->ui->cfgTab->currentIndex() == 2){
        if(event->key() == Qt::Key_Control){
            this->pD->close();
            this->pD = nullptr;
            this->releaseKeyboard();
        }else{
            event->ignore();
        }
    }
}

void EditCfgDialog::clickHandlerGR(int row, int col){
    unsigned int r, p, l; //Register (1-35), position (0-15), length
    for(int i = 0; i<this->ui->GRCombo->count(); i+=1){
        r = this->ui->GRCombo->itemData(i).value<Fei4RegHelper>().getMOffset();
        if(r-1 == row){
            p = this->ui->GRCombo->itemData(i).value<Fei4RegHelper>().getBOffset();
            l = this->ui->GRCombo->itemData(i).value<Fei4RegHelper>().getMask();
            if(p <= col && p+l > col){
                this->ui->GRCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void EditCfgDialog::normalizeGRColors(){
    for(unsigned int row = 0; row<35; row+=1){
        for(unsigned int col = 0; col<16; col+=1){
            this->ui->GRTable->item(row, col)->setBackgroundColor(this->grey);
        }
    }

    unsigned int r, p, l; bool m; //register (1-35), position (0-15), length, msbright
    for(int i = 0; i<this->ui->GRCombo->count(); i+=1){
        r = this->ui->GRCombo->itemData(i).value<Fei4RegHelper>().getMOffset();
        p = this->ui->GRCombo->itemData(i).value<Fei4RegHelper>().getBOffset();
        l = this->ui->GRCombo->itemData(i).value<Fei4RegHelper>().getMask();
        m = this->ui->GRCombo->itemData(i).value<Fei4RegHelper>().getMsbRight();
        for(int j = 0; j<l; j+=1){
            this->ui->GRTable->item(r-1, p+j)->setBackgroundColor(this->darkyellow);
        }
        m ? this->ui->GRTable->item(r-1, p+l-1)->setBackgroundColor(this->lightyellow) :
            this->ui->GRTable->item(r-1, p)->setBackgroundColor(this->lightyellow);
    }

    r = this->ui->GRCombo->itemData(this->ui->GRCombo->currentIndex()).value<Fei4RegHelper>().getMOffset();
    p = this->ui->GRCombo->itemData(this->ui->GRCombo->currentIndex()).value<Fei4RegHelper>().getBOffset();
    l = this->ui->GRCombo->itemData(this->ui->GRCombo->currentIndex()).value<Fei4RegHelper>().getMask();
    m = this->ui->GRCombo->itemData(this->ui->GRCombo->currentIndex()).value<Fei4RegHelper>().getMsbRight();
    for(int i = 0; i<l; i+=1){
        this->ui->GRTable->item(r-1, p+i)->setBackgroundColor(this->darkgreen);
    }
    m ? this->ui->GRTable->item(r-1, p+l-1)->setBackgroundColor(this->lightgreen):
        this->ui->GRTable->item(r-1, p)->setBackgroundColor(this->lightgreen);
}

void EditCfgDialog::normalizeHandler(QString qS){
    int newIndex = this->ui->GRCombo->currentIndex();
    this->normalizeGRColors();
    this->ui->GRSpin->setValue((int)this->j["FE-I4B"]["GlobalConfig"][qS.toStdString()]);
    this->ui->GRSpin->setMaximum((int)std::pow(2.0, (double)this->ui->GRCombo->itemData(newIndex).value<Fei4RegHelper>().getMask()) - 1);
}

void EditCfgDialog::updateHandlerGR(int i){
    this->j["FE-I4B"]["GlobalConfig"][this->ui->GRCombo->currentText().toStdString()] = i;
}

void EditCfgDialog::on_gCfgZoomInButton_clicked(){
    for(unsigned int i = 0; i<35; i+=1){
        this->ui->GRTable->setRowHeight(i, this->ui->GRTable->rowHeight(i) + 1);
    }
    for(unsigned int i = 0; i<16; i+=1){
        this->ui->GRTable->setColumnWidth(i, this->ui->GRTable->columnWidth(i) + 4);
    }
}

void EditCfgDialog::on_gCfgZoomOutButton_clicked(){
    if(this->ui->GRTable->rowHeight(0) == 1){
        return;
    }
    for(unsigned int i = 0; i<35; i+=1){
        this->ui->GRTable->setRowHeight(i, this->ui->GRTable->rowHeight(i) - 1);
    }
    for(unsigned int i = 0; i<16; i+=1){
        this->ui->GRTable->setColumnWidth(i, this->ui->GRTable->columnWidth(i) - 4);
    }

    return;
}

void EditCfgDialog::on_chipIdSpin_valueChanged(int arg1){
    this->j["FE-I4B"]["Parameter"]["chipId"] = arg1;
}


void EditCfgDialog::on_lCapDSpin_valueChanged(double arg1){
    this->j["FE-I4B"]["Parameter"]["lCap"] = arg1;
}


void EditCfgDialog::on_sCapDSpin_valueChanged(double arg1){
    this->j["FE-I4B"]["Parameter"]["sCap"] = arg1;
}


void EditCfgDialog::on_vcalOffsetDSpin_valueChanged(double arg1){
    this->j["FE-I4B"]["Parameter"]["vcalOffset"] = arg1;
}


void EditCfgDialog::on_vcalSlopeDSpin_valueChanged(double arg1){
    this->j["FE-I4B"]["Parameter"]["vcalOffset"] = arg1;
}

void EditCfgDialog::on_chipNameLine_textChanged(const QString &arg1){
    this->j["FE-I4B"]["name"] = arg1.toStdString();
}

void EditCfgDialog::on_txChannelSpin_valueChanged(int arg1){
    this->j["FE-I4B"]["txChannel"] = arg1;
}

void EditCfgDialog::on_rxChannelSpin_valueChanged(int arg1){
    this->j["FE-I4B"]["rxChannel"] = arg1;
}
