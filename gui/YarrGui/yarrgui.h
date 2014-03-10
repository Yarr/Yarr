#ifndef YARRGUI_H
#define YARRGUI_H

#include <QMainWindow>
#include <QDir>

#include <string>
#include <vector>
#include <cmath>

#include "SpecController.h"

namespace Ui {
class YarrGui;
}

class YarrGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit YarrGui(QWidget *parent = 0);
    ~YarrGui();

private slots:

    void on_device_comboBox_currentIndexChanged(const QString &arg1);

    void on_device_comboBox_currentIndexChanged(int index);

    void on_pushButton_clicked();

private:
    Ui::YarrGui *ui;
    QDir devicePath;
    QStringList deviceList;

    void init();

    std::vector<SpecController*> specVec;
};

#endif // YARRGUI_H
