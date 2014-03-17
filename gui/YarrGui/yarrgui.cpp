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
    for(int i=0; i<deviceList.size(); i++)
        specVec.push_back(new SpecController());
    
    // Init Benchmark Plot
    writeGraph = ui->benchmark_plot->addGraph();
    readGraph = ui->benchmark_plot->addGraph();
    ui->benchmark_plot->xAxis->setLabel("Package size [kB]");
    ui->benchmark_plot->yAxis->setLabel("Speed [MB/s]");
    ui->benchmark_plot->xAxis->setRange(0, 500);
    ui->benchmark_plot->yAxis->setRange(0, 500);
    ui->benchmark_plot->setInteraction(QCP::iRangeDrag, true);
    ui->benchmark_plot->setInteraction(QCP::iRangeZoom, true);
    ui->benchmark_plot->setInteraction(QCP::iSelectPlottables, true);
    ui->benchmark_plot->setInteraction(QCP::iSelectAxes, true);
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
    
    std::cout << "size: " << size << std::endl;
    char *buffer = new char[size];
    file.seekg(0, std::ios::beg);
    file.read(buffer, size);
    int wrote = specVec[index]->progFpga(buffer, size);
    if (wrote != size) {
        QMessageBox errorBox;
        errorBox.critical(0, "Error", "FPGA not succesfully programmed!");
    }
    delete buffer;
    file.close();    
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

    writeGraph->clearData();

    for (unsigned int index=0; index<deviceList.size(); index++) {
        if (specVec[index]->isInitialized()) {
            for (unsigned i=min; i<max; i+=interval) {
                double speed = BenchmarkTools::measureWriteSpeed(specVec[index], i*256, repetitions);
                std::cout << "Starting: " << i << " " << speed << " " << ((steps*interval)/(double)(i-min))*100 << std::endl;
                writeGraph->addData(i, speed);
                ui->benchmark_plot->rescaleAxes();
                ui->benchmark_plot->replot();
                ui->benchmark_progressBar->setValue(((i-min)/(double)(steps*interval))*100);
                QApplication::processEvents(); // Else we lookk like we are not responding
            }
            ui->benchmark_progressBar->setValue(100);
        }
    }       
}
