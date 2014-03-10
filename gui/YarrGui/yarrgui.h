#ifndef YARRGUI_H
#define YARRGUI_H

#include <QMainWindow>
#include <QDir>

#include <string>
#include <cmath>

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

private:
    Ui::YarrGui *ui;
    QDir devicePath;
};

#endif // YARRGUI_H
