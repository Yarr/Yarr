#include "editcfgdialog.h"
#include "ui_editcfgdialog.h"

EditCfgDialog::EditCfgDialog(Fei4 * f, QString cfgFNJ_param, QWidget * parent) :
    QDialog(parent),
    ui(new Ui::EditCfgDialog),
    fE(f),
    cfgFNJ(cfgFNJ_param)
{
    ui->setupUi(this);

    this->fE->toFileJson(this->j);

    ui->cfgTable->setRowCount(336);
    ui->cfgTable->setColumnCount(80);
    for(unsigned int i = 0; i < 336; i += 1){
        ui->cfgTable->setRowHeight(i, 10);
    }
    for(unsigned int i = 0; i < 80; i += 1){
        ui->cfgTable->setColumnWidth(i, 15);
    }
    ui->tableLegend->setMaximumHeight(52);
    ui->cfgTable->horizontalHeader()->hide();
    ui->tableLegend->horizontalHeader()->hide();
    ui->cfgTable->verticalHeader()->hide();
    ui->tableLegend->verticalHeader()->hide();
    QFont* zeroFont = new QFont;
    zeroFont->setPixelSize(1);
    ui->cfgTable->setFont(*zeroFont);
    ui->cfgTable->setMouseTracking(true);
    QObject::connect(ui->cfgTable, SIGNAL(cellClicked(int,int)), this, SLOT(clickHandler(int,int)));
    QObject::connect(ui->cfgTable, SIGNAL(cellEntered(int,int)), this, SLOT(enterHandler(int,int)));
    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->EnableRadio->text().toStdString().c_str()][k];
//            QPushButton * b = new QPushButton(QString::number(l), this);
//            b->setMinimumWidth(10);
//            ui->cfgTable->setCellWidget(i, k, b);
            ui->cfgTable->setItem(i, k, new QTableWidgetItem(QString::number(l)));
            ui->cfgTable->item(i, k)->setBackgroundColor(*(this->fetchColor(l, 1)));
            ui->cfgTable->item(i, k)->setFlags(0x0);
/*            QObject::connect(ui->cfgTable, SIGNAL(cellClicked(int,int)), this, [=](int rowCl, int colCl){
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
                unsigned int u = ui->cfgTable->item(rowCl, colCl)->text().toUInt();
                u = (u+1)%(max_val+1);
                ui->cfgTable->item(rowCl, colCl)->setBackgroundColor(*(this->fetchColor(u, max_val)));
                ui->cfgTable->item(rowCl, colCl)->setText(QString::number(u));
                this->j["FE-I4B"]["PixelConfig"][i][tmpStdStr.c_str()][k] = u;
            });*/
            /*            QObject::connect(b, &QPushButton::clicked, this, [=](){
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
                            unsigned int u = b->text().toUInt();
                            u += 1;
                            if(u > max_val){
                                u = 0;
                            }
                            b->setText(QString::number(u));
                            this->j["FE-I4B"]["PixelConfig"][i][tmpStdStr.c_str()][k] = u;
                        });*/
        }
    }

    this->setWindowState(Qt::WindowMaximized);
    this->initLegend();
}

EditCfgDialog::~EditCfgDialog(){
    delete ui;
}

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
    QString filename = QFileDialog::getSaveFileName(this, tr("Select JSON config file"), "./util/", tr("JSON Config File(*.js *.json)"));
    QFile oF(filename);
    oF.open(QIODevice::WriteOnly);
    QString qS;
    qS = QString::fromStdString(this->j.dump(4));
    QTextStream oS(&oF);
    oS << qS;
    oF.close();

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
    unsigned int u = ui->cfgTable->item(rowCl, colCl)->text().toUInt();
    u = (u+1)%(maxVal.second+1);
    ui->cfgTable->item(rowCl, colCl)->setBackgroundColor(*(this->fetchColor(u, maxVal.second)));
    ui->cfgTable->item(rowCl, colCl)->setText(QString::number(u));
    this->j["FE-I4B"]["PixelConfig"][rowCl][maxVal.first.c_str()][colCl] = u;

    return;
}

void EditCfgDialog::enterHandler(int rowEn, int colEn){
    QString tmpQStr = "(" + QString::number(rowEn) + ", " + QString::number(colEn) + ")";

    ui->linePosInfo->setText(tmpQStr);

    return;
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

    return;
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

    return;
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
