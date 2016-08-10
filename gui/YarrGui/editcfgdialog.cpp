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
    for(unsigned int i = 0; i < 80; i += 1){
        ui->cfgTable->setColumnWidth(i, 25);
    }
    ui->cfgTable->setAlternatingRowColors(true);
    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->EnableRadio->text().toStdString().c_str()][k];
            QPushButton * b = new QPushButton(QString::number(l), this);
            b->setMinimumWidth(10);
            ui->cfgTable->setCellWidget(i, k, b);
            QObject::connect(b, &QPushButton::clicked, this, [=](){
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
                std::cout << tmpStdStr << std::endl; //DEBUG
                this->j["FE-I4B"]["PixelConfig"][i][tmpStdStr.c_str()][k] = u;
            });
        }
    }
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

void EditCfgDialog::on_EnableRadio_clicked(){
    if(!ui->EnableRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->EnableRadio->text().toStdString().c_str()][k];
            QPushButton * b = dynamic_cast<QPushButton*>(ui->cfgTable->cellWidget(i, k));
            b->setText(QString::number(l));
        }
    }

    return;
}

void EditCfgDialog::on_TDACRadio_clicked(){
    if(!ui->TDACRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->TDACRadio->text().toStdString().c_str()][k];
            QPushButton * b = dynamic_cast<QPushButton*>(ui->cfgTable->cellWidget(i, k));
            b->setText(QString::number(l));
        }
    }

    return;
}

void EditCfgDialog::on_LCapRadio_clicked(){
    if(!ui->LCapRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->LCapRadio->text().toStdString().c_str()][k];
            QPushButton * b = dynamic_cast<QPushButton*>(ui->cfgTable->cellWidget(i, k));
            b->setText(QString::number(l));
        }
    }

    return;
}

void EditCfgDialog::on_SCapRadio_clicked(){
    if(!ui->SCapRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->SCapRadio->text().toStdString().c_str()][k];
            QPushButton * b = dynamic_cast<QPushButton*>(ui->cfgTable->cellWidget(i, k));
            b->setText(QString::number(l));
        }
    }

    return;
}

void EditCfgDialog::on_HitbusRadio_clicked(){
    if(!ui->HitbusRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->HitbusRadio->text().toStdString().c_str()][k];
            QPushButton * b = dynamic_cast<QPushButton*>(ui->cfgTable->cellWidget(i, k));
            b->setText(QString::number(l));
        }
    }

    return;
}

void EditCfgDialog::on_FDACRadio_clicked(){
    if(!ui->FDACRadio->isChecked()){
        return;
    }

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->FDACRadio->text().toStdString().c_str()][k];
            QPushButton * b = dynamic_cast<QPushButton*>(ui->cfgTable->cellWidget(i, k));
            b->setText(QString::number(l));
        }
    }

    return;
}
