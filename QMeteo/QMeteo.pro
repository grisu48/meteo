#-------------------------------------------------
#
# Project created by QtCreator 2017-06-27T14:19:38
#
#-------------------------------------------------

QT       += core gui

LIBS += -lmosquitto

CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QMeteo
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qmosquitto.cpp \
    qweatherdata.cpp

HEADERS  += mainwindow.h \
    qmosquitto.h \
    qweatherdata.h \
    json.hpp

FORMS    += mainwindow.ui qweatherdata.ui
