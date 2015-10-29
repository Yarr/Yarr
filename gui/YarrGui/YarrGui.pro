#-------------------------------------------------
#
# Project created by QtCreator 2014-03-07T13:55:18
#
#-------------------------------------------------

QT       += core gui testlib
#macx {
#    QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
#    QMAKE_LFLAGS += -std=c++11 -stdlib=libc++
#    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
#}

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LFLAGS += -std=c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = YarrGui
TEMPLATE = app


SOURCES += main.cpp\
        yarrgui.cpp \
    qcustomplot.cpp \
    plotdialog.cpp

HEADERS  += yarrgui.h \
    qcustomplot.h \
    qdebugstream.h \
    plotdialog.h

FORMS    += yarrgui.ui \
    plotdialog.ui

INCLUDEPATH += ../../src/libSpec/include
INCLUDEPATH += ../../src/libUtil/include
INCLUDEPATH += ../../src/libYarr/include
INCLUDEPATH += ../../src/libFei4/include

LIBS += -L../../src/lib -lfei4 -lspec -lutil -lyarr
