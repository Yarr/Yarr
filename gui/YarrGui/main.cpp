#include "yarrgui.h"
#include <QApplication>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    YarrGui w;
    w.showMaximized();

    return a.exec();
}
