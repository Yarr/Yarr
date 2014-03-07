#ifndef YARRGUI_H
#define YARRGUI_H

#include <QMainWindow>
#include <QDir>
#include <QListWidgetItem>

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
    void on_device_listView_clicked(const QModelIndex &index);

    void on_device_listWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

private:
    Ui::YarrGui *ui;
    QDir devicePath;
};

#endif // YARRGUI_H
