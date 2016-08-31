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

QMAKE_CXXFLAGS += -std=c++11 -Wno-unused-parameter
QMAKE_LFLAGS += -std=c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = YarrGui
TEMPLATE = app


SOURCES += main.cpp\
    yarrgui.cpp \
    yarrgui_spec.cpp \
    yarrgui_fes.cpp \
    yarrgui_scans.cpp \
    yarrgui_plots.cpp \
    qcustomplot.cpp \
    plotdialog.cpp \
    benchmarkdialog.cpp \
    eepromdialog.cpp \
    createscandialog.cpp \
    scanstruct.cpp \
    fei4reghelper.cpp

HEADERS  += yarrgui.h \
    qcustomplot.h \
    qdebugstream.h \
    plotdialog.h \
    benchmarkdialog.h \
    eepromdialog.h \
    createscandialog.h \
    scanstruct.h \
    fei4reghelper.h

FORMS    += yarrgui.ui \
    plotdialog.ui \
    benchmarkdialog.ui \
    eepromdialog.ui \
    createscandialog.ui

INCLUDEPATH += ../../src/libSpec/include
INCLUDEPATH += ../../src/libUtil/include
INCLUDEPATH += ../../src/libYarr/include
INCLUDEPATH += ../../src/libFei4/include

LIBS += -L../../src/lib -lyarr
