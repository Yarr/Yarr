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

EEPROMDialog::~EEPROMDialog()
{
    delete ui;
}

void EEPROMDialog::on_sbefile_button_2_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Select EEPROM content file"), "./", tr("SpecBoard EEPROM content file(*.sbe)"));
    std::fstream file(filename.toStdString().c_str(), std::fstream::in);
    if (!file) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "A problem occured. ");
        ui->sbefile_name->setText("");
        return;
    }
    ui->sbefile_name->setText(filename);
    return;
}

void EEPROMDialog::on_SBEReadButton_clicked()
{
    int index = parentCast->getDeviceComboBoxCurrentIndex();
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    std::string fnKeyword;
    fnKeyword = ui->filename_keyword->text().toStdString().c_str();

    parentCast->specVecAt(index)->readEeprom(buffer, ARRAYLENGTH);
    parentCast->specVecAt(index)->createSbeFile(fnKeyword, buffer, ARRAYLENGTH);

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
        for(unsigned int i = 0; i<(ARRAYLENGTH / 6); i++) {
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

void EEPROMDialog::on_SBEWriteButton_clicked()
{
    int index = parentCast->getDeviceComboBoxCurrentIndex();
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    std::string pathname;
    if (!(ui->SBECheckBox->isChecked())) {
        pathname = ui->sbefile_name->text().toStdString().c_str();
    } else {
        pathname = "util/tmp.sbe";
        QFile tmpFile(QString::fromStdString(pathname));
        tmpFile.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream tmpFileStream(&tmpFile);
        tmpFileStream << ui->SBETextEdit->toPlainText();
        tmpFile.close();
    }

    parentCast->specVecAt(index)->getSbeFile(pathname, buffer, ARRAYLENGTH);
    parentCast->specVecAt(index)->writeEeprom(buffer, ARRAYLENGTH, 0);

    delete buffer;
    return;
}
