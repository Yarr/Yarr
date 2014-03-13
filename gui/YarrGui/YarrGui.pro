#-------------------------------------------------
#
# Project created by QtCreator 2014-03-07T13:55:18
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = YarrGui
TEMPLATE = app


SOURCES += main.cpp\
        yarrgui.cpp \
    qcustomplot.cpp

HEADERS  += yarrgui.h \
    qcustomplot.h \
    qdebugstream.h

FORMS    += yarrgui.ui

INCLUDEPATH += ../../src/libSpec/include SpecController.h
LIBS += -L../../src/lib -lspec -lutil -lyarr
