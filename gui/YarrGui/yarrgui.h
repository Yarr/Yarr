#ifndef YARRGUI_H
#define YARRGUI_H

#include <QMainWindow>

namespace Ui {
class YarrGui;
}

class YarrGui : public QMainWindow
{
    Q_OBJECT

public:
    explicit YarrGui(QWidget *parent = 0);
    ~YarrGui();

private:
    Ui::YarrGui *ui;
};

#endif // YARRGUI_H
