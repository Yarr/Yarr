#include "yarrgui.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    YarrGui w;
    w.show();

    return a.exec();
}
