#include "yarrgui.h"
#include "ui_yarrgui.h"

#include <BitFile.h>
#include <BenchmarkTools.h>

#include <iostream>
#include <fstream>
#include <string>

#include <QMessageBox>

YarrGui::YarrGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::YarrGui)
{
    ui->setupUi(this);

    this->init();

    // Get list of devices
    devicePath.setPath("/dev/");
    QStringList filter;
    filter << "spec*";
    devicePath.setFilter(QDir::System);
    deviceList = devicePath.entryList(filter);
    
    // Display devices in list
    ui->device_comboBox->addItems(deviceList);

    // Init device vector
    for(int i=0; i<deviceList.size(); i++) {
        specVec.push_back(new SpecController());
    }

    // Init Benchmark Plot
    ui->benchmark_plot->xAxis->setLabel("Package size [kB]");
    ui->benchmark_plot->yAxis->setLabel("Speed [MB/s]");
    ui->benchmark_plot->xAxis->setRange(0, 500);
    ui->benchmark_plot->yAxis->setRange(0, 500);
    ui->benchmark_plot->setInteraction(QCP::iRangeDrag, true);
    ui->benchmark_plot->setInteraction(QCP::iRangeZoom, true);
    ui->benchmark_plot->setInteraction(QCP::iSelectPlottables, true);
    ui->benchmark_plot->setInteraction(QCP::iSelectAxes, true);
    ui->benchmark_plot->legend->setVisible(true);
    ui->benchmark_plot->legend->setFont(QFont("Helvetica", 9));
    ui->benchmark_plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignRight|Qt::AlignBottom);

    QPen pen;
    QColor color;
    for(int i=0; i<deviceList.size(); i++) {
        writeGraphVec.push_back(ui->benchmark_plot->addGraph()); 
        readGraphVec.push_back(ui->benchmark_plot->addGraph());
        
        QColor color1(sin(i*0.3)*100+100, sin(i*0.6+0.7)*100+100, sin(i*0.4+0.6)*100+100);
        pen.setColor(color1);
        writeGraphVec[i]->setPen(pen);
        writeGraphVec[i]->setName("Spec #" + QString::number(i) + " DMA Write");
        writeGraphVec[i]->setLineStyle(QCPGraph::lsLine);
        writeGraphVec[i]->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color1, 5));
 
        QColor color2(sin(i*0.6+0.7)*100+100, sin(i*0.3)*100+100, sin(i*0.4+0.6)*100+100);
        pen.setColor(color2);
        readGraphVec[i]->setPen(pen);
        readGraphVec[i]->setName("Spec #" + QString::number(i) + " DMA Read");
        readGraphVec[i]->setLineStyle(QCPGraph::lsLine);;
        readGraphVec[i]->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, color2, 5));
    }

    // Init console
    qout = new QDebugStream(std::cout, ui->console, QColor("black"));
    qerr = new QDebugStream(std::cerr, ui->console, QColor("red"));

    ui->main_tabWidget->setCurrentIndex(0);
    ui->main_tabWidget->setTabEnabled(0, true);
    ui->main_tabWidget->setTabEnabled(1, false);
}

YarrGui::~YarrGui()
{
    // Clean up devices
    for(int i=0; i<deviceList.size(); i++) {
        if (specVec[i]->isInitialized())
            delete specVec[i];
    }

    delete ui;
}

void YarrGui::on_device_comboBox_currentIndexChanged(int index)
{
    if (specVec.size()) {
        ui->specid_value->setNum(specVec[index]->getId());
        ui->bar0_value->setNum(specVec[index]->getBarSize(0));
        ui->bar4_value->setNum(specVec[index]->getBarSize(4));
    }
}


void YarrGui::init() {
    // Preset labels
    ui->specid_value->setNum(-1);
    ui->bar0_value->setNum(-1);
    ui->bar4_value->setNum(-1);
}

void YarrGui::on_init_button_clicked()
{
    int index = ui->device_comboBox->currentIndex();
    if (specVec.size() == 0 || index > specVec.size()) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Device not found!");
    } else {
        specVec[index]->init(index);
        if (specVec[index]->isInitialized()) {
            ui->specid_value->setNum(specVec[index]->getId());
            ui->bar0_value->setNum(specVec[index]->getBarSize(0));
            ui->bar4_value->setNum(specVec[index]->getBarSize(4));
            ui->main_tabWidget->setTabEnabled(1, true);
        } else {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Initialization not successful!");
        }
    }
}

void YarrGui::on_progfile_button_clicked() {
    QString filename = QFileDialog::getOpenFileName(this, tr("Select bit-file"), "./", tr("Bit File(*.bit)"));
    std::fstream file(filename.toStdString().c_str(), std::fstream::in);
    if (!file && BitFile::checkFile(file)) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Selected bit file looks bad!");
        ui->progfile_name->setText("");
        return;
    }
    ui->progfile_name->setText(filename);
}

void YarrGui::on_prog_button_clicked() {
    
    int index = ui->device_comboBox->currentIndex();
    if (specVec.size() == 0 || index > specVec.size()) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "Device not found!");
    } else {
        if (!specVec[index]->isInitialized()) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Spec not initiliazed!");
            return;
        }

        std::fstream file(ui->progfile_name->text().toStdString().c_str(), std::fstream::in);
        if (!file) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "Problem opening File!");
            return;
        }

        size_t size = BitFile::getSize(file);
        
        // Read file into buffer
        char *buffer = new char[size];
        file.seekg(0, std::ios::beg);
        file.read(buffer, size);
        
        // Program FPGA
        int wrote = specVec[index]->progFpga(buffer, size);
        if (wrote != size) {
            QMessageBox errorBox;
            errorBox.critical(0, "Error", "FPGA not succesfully programmed!");
        }
        delete buffer;
        file.close();    
    }
}

void YarrGui::on_minSize_spinBox_valueChanged(int i) {
    ui->maxSize_spinBox->setMinimum(i+1);
}

void YarrGui::on_maxSize_spinBox_valueChanged(int i) {
    ui->minSize_spinBox->setMaximum(i-1);
}

void YarrGui::on_startWrite_button_clicked() {
    unsigned min = ui->minSize_spinBox->value();
    unsigned max = ui->maxSize_spinBox->value();
    unsigned steps = ui->steps_spinBox->value();
    unsigned repetitions = ui->repetitions_spinBox->value();

    unsigned interval = (max-min)/steps;

    for (unsigned int index=0; index<deviceList.size(); index++) {
        writeGraphVec[index]->clearData();
        
        if (specVec[index]->isInitialized()) {
            for (unsigned i=min; i<=max; i+=interval) {
                double speed = BenchmarkTools::measureWriteSpeed(specVec[index], i*256, repetitions);
                if (speed < 0) {
                    QMessageBox errorBox;
                    errorBox.critical(0, "Error", "DMA timed out!");
                    return;
                }
                writeGraphVec[index]->addData(i, speed);
                ui->benchmark_plot->rescaleAxes();
                ui->benchmark_plot->replot();
                double per = ((i-min)/(double)interval)/(double)steps;
                ui->benchmark_progressBar->setValue(per*100);
                QApplication::processEvents(); // Else we look like we are not responding
            }
        }
    }       
}

void YarrGui::on_startRead_button_clicked() {
    unsigned min = ui->minSize_spinBox->value();
    unsigned max = ui->maxSize_spinBox->value();
    unsigned steps = ui->steps_spinBox->value();
    unsigned repetitions = ui->repetitions_spinBox->value();

    unsigned interval = (max-min)/steps;

    for (unsigned int index=0; index<deviceList.size(); index++) {
        readGraphVec[index]->clearData();
        
        if (specVec[index]->isInitialized()) {
            for (unsigned i=min; i<=max; i+=interval) {
                double speed = BenchmarkTools::measureReadSpeed(specVec[index], i*256, repetitions);
                if (speed < 0) {
                    QMessageBox errorBox;
                    errorBox.critical(0, "Error", "DMA timed out!");
                    return;
                }
                readGraphVec[index]->addData(i, speed);
                ui->benchmark_plot->rescaleAxes();
                ui->benchmark_plot->replot();
                double per = ((i-min)/(double)interval)/(double)steps;
                ui->benchmark_progressBar->setValue(per*100);
                QApplication::processEvents(); // Else we look like we are not responding
            }
        }
    }       
}

void YarrGui::on_NoiseScanButton_clicked()
{

}

void YarrGui::on_sbefile_button_2_clicked()
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
}

void YarrGui::on_SBEWriteButton_clicked()
{
    int index = ui->device_comboBox->currentIndex();
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

    specVec.at(index)->getSbeFile(pathname, buffer, ARRAYLENGTH);
    specVec.at(index)->writeEeprom(buffer, ARRAYLENGTH, 0);

    delete buffer;

}

void YarrGui::on_SBEReadButton_clicked()
{
    int index = ui->device_comboBox->currentIndex();
    uint8_t * buffer = new uint8_t[ARRAYLENGTH];
    std::string fnKeyword;
    fnKeyword = ui->filename_keyword->text().toStdString().c_str();

    specVec.at(index)->readEeprom(buffer, ARRAYLENGTH);
    specVec.at(index)->createSbeFile(fnKeyword, buffer, ARRAYLENGTH);

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

}
