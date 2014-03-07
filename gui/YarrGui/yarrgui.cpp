#include "yarrgui.h"
#include "ui_yarrgui.h"

YarrGui::YarrGui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::YarrGui)
{
    ui->setupUi(this);
}

YarrGui::~YarrGui()
{
    delete ui;
}
