#ifndef YARRGUI_H
#define YARRGUI_H

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QColor>

#include <string>
#include <vector>
#include <cmath>

#include "qdebugstream.h"
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

    void on_init_button_clicked();

    void on_progfile_button_clicked();

private:
    Ui::YarrGui *ui;
    QDir devicePath;
    QStringList deviceList;
    QDebugStream *qout;
    QDebugStream *qerr;

    void init();

    std::vector<SpecController*> specVec;
};

#endif // YARRGUI_H
