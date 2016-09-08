#include "eepromdialog.h"
#include "ui_eepromdialog.h"

EEPROMDialog::EEPROMDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EEPROMDialog)
    {
        ui->setupUi(this);

        parentCast = dynamic_cast<YarrGui*>(parent);
        if(parentCast == nullptr) {
            std::cerr << "Parent cast failed. Aborting...\n";
            exit(-1);
        }

}

EEPROMDialog::~EEPROMDialog(){
    delete ui;
}

void EEPROMDialog::on_wrFromEditButton_clicked(){
    int index = parentCast->getDeviceComboBoxCurrentIndex();
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    std::string pathname;

    pathname = "util/tmp.sbe";
    QFile tmpFile(QString::fromStdString(pathname));
    tmpFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream tmpFileStream(&tmpFile);
    tmpFileStream << ui->SBETextEdit->toPlainText();
    tmpFile.close();

    parentCast->specVecAt(index)->getSbeFile(pathname, buffer, ARRAYLENGTH);
    parentCast->specVecAt(index)->writeEeprom(buffer, ARRAYLENGTH, 0);

    delete buffer;
    return;
}

void EEPROMDialog::on_SBEReadButton_clicked(){
    int index = parentCast->getDeviceComboBoxCurrentIndex();
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    std::string fName;
    fName = ui->sbefile_name_3->text().toStdString();

    parentCast->specVecAt(index)->readEeprom(buffer, ARRAYLENGTH);
    parentCast->specVecAt(index)->createSbeFile(fName, buffer, ARRAYLENGTH);

    //TODO make prittier, too much code duplication
    std::stringstream contentStream;

    contentStream << std::hex;
    contentStream << std::showbase;
    contentStream << std::setw(9) << "addr" << std::setw(5) << "msk" << std::setw(12) << "data" << std::endl;
    //256/6 = 42; 256%6 = 4
    {
        uint16_t a;     //address
        uint8_t  m;     //mask
        uint32_t d;     //data
        for(unsigned int i = 0; i<(ARRAYLENGTH / 6); i++){
            a  = ((buffer[i*6] | (buffer[i*6+1] << 8)) & 0xffc);
            m  = ((buffer[i*6+1] & 0xf0) >> 4);
            d  = (buffer[i*6+2] | (buffer[i*6+3] << 8) | (buffer[i*6+4] << 16) | (buffer[i*6+5] << 24));
            contentStream << std::setw(9) << a << std::setw(5) << (int)m << std::setw(12) << d << std::endl;
        }
    }
    contentStream << std::dec;
    contentStream << std::noshowbase;

    QString content = QString::fromStdString(contentStream.str());

    ui->SBETextEdit->setText(content);

    delete buffer;
    return;
}

void EEPROMDialog::on_SBEWriteButton_clicked(){
    int index = parentCast->getDeviceComboBoxCurrentIndex();
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    std::string pathname;

    pathname = ui->sbefile_name->text().toStdString();

    parentCast->specVecAt(index)->getSbeFile(pathname, buffer, ARRAYLENGTH);
    parentCast->specVecAt(index)->writeEeprom(buffer, ARRAYLENGTH, 0);

    delete buffer;
    return;
}

void EEPROMDialog::on_sbefile_button_2_clicked(){
    QString filename = QFileDialog::getOpenFileName(this,
                                                    "Select EEPROM content file",
                                                    "./",
                                                    "SpecBoard EEPROM content file(*.sbe)");
    ui->sbefile_name->setText(filename);
    return;
}

void EEPROMDialog::on_sbefile_button_4_clicked(){
    QString filename = QFileDialog::getSaveFileName(this,
                                                    "Select EEPROM content file",
                                                    "./",
                                                    "SpecBoard EEPROM content file(*.sbe)");
    ui->sbefile_name_3->setText(filename);
    return;
}

