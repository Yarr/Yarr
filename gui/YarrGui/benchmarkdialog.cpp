#include "benchmarkdialog.h"
#include "ui_benchmarkdialog.h"

BenchmarkDialog::BenchmarkDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BenchmarkDialog)
{
    parentCast = dynamic_cast<YarrGui*>(parent);
    if(parentCast == nullptr) {
        std::cerr << "Parent cast failed. Aborting...\n";
        this->close();
    }

    ui->setupUi(this);
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
    for(int i=0; i<parentCast->getDeviceListSize(); i++) {
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

    QList<int> tmpList;
    tmpList.append(2500);
    tmpList.append(5000);
    ui->splitter->setSizes(tmpList);
}

BenchmarkDialog::~BenchmarkDialog()
{
    delete ui;
}

void BenchmarkDialog::on_startWrite_button_clicked() {
    unsigned min = ui->minSize_spinBox->value();
    unsigned max = ui->maxSize_spinBox->value();
    unsigned steps = ui->steps_spinBox->value();
    unsigned repetitions = ui->repetitions_spinBox->value();

    unsigned interval = (max-min)/steps;

    for (unsigned int index=0; index<parentCast->getDeviceListSize(); index++) {
        writeGraphVec[index]->clearData();

        if (parentCast->isSpecInitialized(index)) {
            for (unsigned i=min; i<=max; i+=interval) {
                double speed = BenchmarkTools::measureWriteSpeed(parentCast->specVecAt(index), i*256, repetitions);
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
    return;
}

void BenchmarkDialog::on_startRead_button_clicked() {
    unsigned min = ui->minSize_spinBox->value();
    unsigned max = ui->maxSize_spinBox->value();
    unsigned steps = ui->steps_spinBox->value();
    unsigned repetitions = ui->repetitions_spinBox->value();

    unsigned interval = (max-min)/steps;

    for (unsigned int index=0; index<parentCast->getDeviceListSize(); index++) {
        readGraphVec[index]->clearData();

        if(parentCast->isSpecInitialized(index)) {
            for (unsigned i=min; i<=max; i+=interval) {
                double speed = BenchmarkTools::measureReadSpeed(parentCast->specVecAt(index), i*256, repetitions);
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
    return;
}
