#-------------------------------------------------
#
# Project created by QtCreator 2016-10-17T20:00:21
#
#-------------------------------------------------

QT       += core gui network xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = meteo
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    receiverthread.cpp

HEADERS  += mainwindow.h \
    receiverthread.h

FORMS    += mainwindow.ui
