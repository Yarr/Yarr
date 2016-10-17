#include "pointerdialog.h"
#include "ui_pointerdialog.h"

PointerDialog::PointerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PointerDialog)
{
    this->parentCast = dynamic_cast<EditCfgDialog*>(parent);
    if(parentCast == nullptr){
        std::cerr << "Severe cast error. Aborting... " << std::endl;
        this->close();
    }
    ui->setupUi(this);
    ui->spinBox->setValue(parentCast->eN);
    switch(parentCast->eM){
        case EditCfgDialog::editMode::ONE:
            ui->oneModeRadio->setChecked(true);
            break;
        case EditCfgDialog::editMode::ROW:
            ui->rowModeRadio->setChecked(true);
            break;
        case EditCfgDialog::editMode::COL:
            ui->colModeRadio->setChecked(true);
            break;
        case EditCfgDialog::editMode::ALL:
            ui->allModeRadio->setChecked(true);
            break;
        case EditCfgDialog::editMode::SONE:
            ui->soneModeRadio->setChecked(true);
            break;
        case EditCfgDialog::editMode::SROW:
            ui->srowModeRadio->setChecked(true);
            break;
        case EditCfgDialog::editMode::SCOL:
            ui->scolModeRadio->setChecked(true);
            break;
        case EditCfgDialog::editMode::SALL:
            ui->sallModeRadio->setChecked(true);
            break;
    }
}

PointerDialog::~PointerDialog()
{
    delete ui;
}

void PointerDialog::on_spinBox_valueChanged(int arg1){
    parentCast->eN = arg1;
}

void PointerDialog::on_oneModeRadio_clicked(){
    if(ui->oneModeRadio->isChecked()){
        this->parentCast->eM = EditCfgDialog::editMode::ONE;
    }
}

void PointerDialog::on_rowModeRadio_clicked(){
    if(ui->rowModeRadio->isChecked()){
        this->parentCast->eM = EditCfgDialog::editMode::ROW;
    }
}

void PointerDialog::on_colModeRadio_clicked(){
    if(ui->colModeRadio->isChecked()){
        this->parentCast->eM = EditCfgDialog::editMode::COL;
    }
}

void PointerDialog::on_allModeRadio_clicked(){
    if(ui->allModeRadio->isChecked()){
        this->parentCast->eM = EditCfgDialog::editMode::ALL;
    }
}

void PointerDialog::on_soneModeRadio_clicked(){
    if(ui->soneModeRadio->isChecked()){
        this->parentCast->eM = EditCfgDialog::editMode::SONE;
    }
}

void PointerDialog::on_srowModeRadio_clicked(){
    if(ui->srowModeRadio->isChecked()){
        this->parentCast->eM = EditCfgDialog::editMode::SROW;
    }
}

void PointerDialog::on_scolModeRadio_clicked(){
    if(ui->scolModeRadio->isChecked()){
        this->parentCast->eM = EditCfgDialog::editMode::SCOL;
    }
}

void PointerDialog::on_sallModeRadio_clicked(){
    if(ui->sallModeRadio->isChecked()){
        this->parentCast->eM = EditCfgDialog::editMode::SALL;
    }
}
