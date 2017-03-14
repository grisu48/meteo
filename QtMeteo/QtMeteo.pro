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
    qcustomplot.cpp \
    qweatherdata.cpp \
    weatherdata.cpp \
    receiver.cpp \
    tcpreceiver.cpp \
    udpreceiver.cpp \
    configfile.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h \
    qweatherdata.h \
    stations.h \
    weatherdata.h \
    receiver.h \
    tcpreceiver.h \
    udpreceiver.h \
    configfile.h

FORMS    += mainwindow.ui \
    qweatherdata.ui
