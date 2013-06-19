#-------------------------------------------------
#
# Project created by QtCreator 2013-06-19T20:38:39
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ADSniffer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp qcustomplot.cpp hled.cpp

HEADERS  += mainwindow.h qcustomplot.h hled.h

FORMS    += mainwindow.ui

include(qextserialport/src/qextserialport.pri)
