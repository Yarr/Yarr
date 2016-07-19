#include "editcfgdialog.h"
#include "ui_editcfgdialog.h"

EditCfgDialog::EditCfgDialog(Fei4 * f, QString cfgFNJ_param, QWidget * parent) :
    QDialog(parent),
    ui(new Ui::EditCfgDialog),
    fE(f),
    cfgFNJ(cfgFNJ_param)
{
    ui->setupUi(this);

    nlohmann::json j;
    this->fE->toFileJson(j);
//    std::string s;
//    s = j.dump(4);
//    ui->cfgEdit->setPlainText(QString::fromStdString(s));

    ui->cfgTable->setRowCount(336);
    ui->cfgTable->setColumnCount(80);
//    ui->cfgTable->setAlternatingRowColors(true);
    std::string tmpStdStr = "";
    if(ui->EnableRadio->isChecked()){
        tmpStdStr = ui->EnableRadio->text().toStdString();
    }else if(ui->TDACRadio->isChecked()){
        tmpStdStr = ui->TDACRadio->text().toStdString();
    }else if(ui->LCapRadio->isChecked()){
        tmpStdStr = ui->LCapRadio->text().toStdString();
    }else if(ui->SCapRadio->isChecked()){
        tmpStdStr = ui->SCapRadio->text().toStdString();
    }else if(ui->HitbusRadio->isChecked()){
        tmpStdStr = ui->HitbusRadio->text().toStdString();
    }else if(ui->FDACRadio->isChecked()){
        tmpStdStr = ui->FDACRadio->text().toStdString();
    }else{
        std::cerr << "No valid radio button checked. Aborting... " << std::endl;
        this->close();
    }
    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->EnableRadio->text().toStdString().c_str()][k];
            QPushButton * b = new QPushButton(QString::number(l), this);
            ui->cfgTable->setCellWidget(i, k, b);
            QObject::connect(b, &QPushButton::clicked, this, [=](){
                unsigned int u = b->text().toUInt();
                u += 1;
                b->setText(QString::number(u));
            });
        }
    }
//    QPushButton * qpshb = dynamic_cast<QPushButton*>(ui->cfgTable->cellWidget(5, 5)); //DEBUG
//    if(qpshb == nullptr){std::cerr << "You need to revise something here. " << std::endl;} //DEBUG
}

EditCfgDialog::~EditCfgDialog(){
    delete ui;
}

void EditCfgDialog::on_saveButton_clicked(){
    nlohmann::json j;
    try{
        j = nlohmann::json::parse(ui->cfgEdit->toPlainText().toStdString());
    }
    catch(std::invalid_argument){
        std::cerr << "ERROR - Malformed argument - aborting" << std::endl;
        return;
    }
    try{
        this->fE->fromFileJson(j);
    }
    catch(std::domain_error){
        std::cerr << "ERROR - Malformed argument - aborting" << std::endl;
        return;
    }
    QFile oF(this->cfgFNJ);
    oF.open(QIODevice::WriteOnly);
    QString qS;
    qS = QString::fromStdString(j.dump(4));
    QTextStream oS(&oF);
    oS << qS;
    oF.close();

    return;
}

void EditCfgDialog::on_applyButton_clicked(){
    nlohmann::json j;
    try{
        j = nlohmann::json::parse(ui->cfgEdit->toPlainText().toStdString());
    }
    catch(std::invalid_argument){
        std::cerr << "ERROR - Malformed config - aborting" << std::endl;
        return;
    }

    try{
        this->fE->fromFileJson(j);
    }
    catch(std::domain_error){
        std::cout << "ERROR - Malformed config - aborting" << std::endl;
        return;
    }

    return;
}

void EditCfgDialog::on_saveAsButton_clicked(){
    QString filename = QFileDialog::getSaveFileName(this, tr("Select JSON config file"), "./util/", tr("JSON Config File(*.js *.json)"));
    nlohmann::json j;
    try{
        j = nlohmann::json::parse(ui->cfgEdit->toPlainText().toStdString());
    }
    catch(std::invalid_argument){
        std::cerr << "ERROR - Malformed config - aborting" << std::endl;
        return;
    }
    QFile oF(filename);
    oF.open(QIODevice::WriteOnly);
    QString qS;
    qS = QString::fromStdString(j.dump(4));
    if(qS.at(qS.size()-1) != '\n'){
        qS.append("\n");
    }
    QTextStream oS(&oF);
    oS << qS;
    oF.close();

    return;
}

void EditCfgDialog::on_EnableRadio_clicked(){
    if(!ui->EnableRadio->isChecked()){
        return;
    }

    nlohmann::json j;
    this->fE->toFileJson(j);

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

    nlohmann::json j;
    this->fE->toFileJson(j);

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

    nlohmann::json j;
    this->fE->toFileJson(j);

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

    nlohmann::json j;
    this->fE->toFileJson(j);

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

    nlohmann::json j;
    this->fE->toFileJson(j);

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

    nlohmann::json j;
    this->fE->toFileJson(j);

    for(unsigned int i = 0; i<336; i += 1){
        for(unsigned int k = 0; k < 80; k += 1){
            unsigned int l = j["FE-I4B"]["PixelConfig"][i][ui->FDACRadio->text().toStdString().c_str()][k];
            QPushButton * b = dynamic_cast<QPushButton*>(ui->cfgTable->cellWidget(i, k));
            b->setText(QString::number(l));
        }
    }

    return;
}
