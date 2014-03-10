#include "yarrgui.h"
#include "ui_yarrgui.h"

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

    // Init device vector
    specVec.resize(deviceList.size());
    for(int i=0; i<deviceList.size(); i++)
        specVec[i] = new SpecController;

    // Display devices in list
    ui->device_comboBox->addItems(deviceList);
}

YarrGui::~YarrGui()
{
    // Clean up devices
    for(int i=0; i<deviceList.size(); i++)
        delete specVec[i];

    delete ui;
}

void YarrGui::on_device_comboBox_currentIndexChanged(const QString &arg1)
{
    //ui->bar0_value->setText(arg1);
}

void YarrGui::on_device_comboBox_currentIndexChanged(int index)
{
    ui->specid_value->setNum(specVec[index]->getId());
    ui->bar0_value->setNum(specVec[index]->getBarSize(0));
    ui->bar4_value->setNum(specVec[index]->getBarSize(4));
}

void YarrGui::init() {
    // Preset labels
    ui->specid_value->setNum(-1);
    ui->bar0_value->setNum(-1);
    ui->bar4_value->setNum(-1);
}

void YarrGui::on_pushButton_clicked()
{
    int index = ui->device_comboBox->currentIndex();
    specVec[index]->init(index);
    ui->specid_value->setNum(specVec[index]->getId());
    ui->bar0_value->setNum(specVec[index]->getBarSize(0));
    ui->bar4_value->setNum(specVec[index]->getBarSize(4));
}
