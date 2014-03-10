#include "yarrgui.h"
#include "ui_yarrgui.h"

YarrGui::YarrGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::YarrGui)
{
    ui->setupUi(this);

    devicePath.setPath("/dev/");
    QStringList filter;
    QStringList deviceList;
    filter << "spec*";
    devicePath.setFilter(QDir::System);
    deviceList = devicePath.entryList(filter);

    ui->device_comboBox->addItems(deviceList);
}

YarrGui::~YarrGui()
{
    delete ui;
}

void YarrGui::on_device_comboBox_currentIndexChanged(const QString &arg1)
{
    ui->bar0_value->setText(arg1);
}

void YarrGui::on_device_comboBox_currentIndexChanged(int index)
{
    ui->specid_value->setNum(index);
}
