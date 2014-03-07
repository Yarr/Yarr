#include "yarrgui.h"
#include "ui_yarrgui.h"

YarrGui::YarrGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::YarrGui)
{
    ui->setupUi(this);

    ui->device_listWidget->setSelectionMode(QListWidget::SingleSelection);

    devicePath.setPath("/dev");
    QStringList filter;
    QStringList deviceList;
    filter << "specDevice*";
    devicePath.setFilter(QDir::System);
    deviceList = devicePath.entryList(filter);
    ui->device_listWidget->addItems(deviceList);

    QVector<double> x(100), y(100);
    for (int i=0; i<100; i++) {
        x[i] = i*0.01;
        y[i] = sin(i*0.01);
    }
    ui->plotWidget->addGraph();
    ui->plotWidget->graph(0)->setData(x, y);
}

YarrGui::~YarrGui()
{
    delete ui;
}

void YarrGui::on_device_listView_clicked(const QModelIndex &index)
{
    ui->selDevice_label->setText(QString(ui->device_listWidget->selectedItems().count()));
}

void YarrGui::on_device_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    ui->selDevice_label->setText(devicePath.currentPath() + current->text());
}
