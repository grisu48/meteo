#-------------------------------------------------
#
# Project created by QtCreator 2016-10-17T20:00:21
#
#-------------------------------------------------

QT       += core gui network xml printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = meteo
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    receiverthread.cpp \
    station.cpp \
    qcustomplot.cpp \
    stationdialog.cpp

HEADERS  += mainwindow.h \
    receiverthread.h \
    station.h \
    qcustomplot.h \
    stationdialog.h

FORMS    += mainwindow.ui \
    stationdialog.ui
