#include "editcfgdialog.h"
#include "ui_editcfgdialog.h"

EditCfgDialog::EditCfgDialog(Fei4 * f, QString cfgFNJ_param, QWidget * parent) :
    QDialog(parent),
    ui(new Ui::EditCfgDialog),
    fE(f),
    cfgFNJ(cfgFNJ_param)
{
    ui->setupUi(this);

/*    std::ifstream iF(cfgFNJ);
    std::streampos fsize = 0;
    fsize = iF.tellg();
    iF.seekg(0, std::ios::end);
    fsize = iF.tellg() - fsize;
    iF.seekg(0, std::ios::beg);
    char buf[size_t(fsize)];
    iF.read(buf, size_t(fsize));
    std::string s(buf);
    while(s.at(s.size()-1) != '}'){
        s.pop_back();
    }
    s.append("\n");
    iF.close();
    ui->cfgEdit->setPlainText(QString::fromStdString(s));
    if(cfgFNJ == "util/default.js"){
        cfgFNJ = "util/tmp.js";
        std::cout << "Saving as " << cfgFNJ << std::endl;
    }*/

    QFile iF(this->cfgFNJ);
    iF.open(QIODevice::ReadOnly);
    QByteArray bA;
    bA = iF.readAll();
    iF.close();
    QString qS(bA);
    ui->cfgEdit->setPlainText(qS);
}

EditCfgDialog::~EditCfgDialog(){
    delete ui;
}

void EditCfgDialog::on_saveButton_clicked(){
    nlohmann::json j;
    j = nlohmann::json::parse(ui->cfgEdit->toPlainText().toStdString());
    QFile oF(this->cfgFNJ);
    oF.open(QIODevice::WriteOnly);
    QString qS;
    qS = QString::fromStdString(j.dump(4));
    QTextStream oS(&oF);
    oS << qS;
    oF.close();
    this->fE->fromFileJson(j);

    return;
}

void EditCfgDialog::on_applyButton_clicked(){
    nlohmann::json j;
    j = nlohmann::json::parse(ui->cfgEdit->toPlainText().toStdString());
    this->fE->fromFileJson(j);

    return;
}

void EditCfgDialog::on_saveAsButton_clicked(){
    QString filename = QFileDialog::getSaveFileName(this, tr("Select JSON config file"), "./util/", tr("JSON Config File(*.js *.json)"));
    nlohmann::json j;
    j = nlohmann::json::parse(ui->cfgEdit->toPlainText().toStdString());
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
