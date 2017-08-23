#-------------------------------------------------
#
# Project created by QtCreator 2017-06-27T14:19:38
#
#-------------------------------------------------

QT       += core gui sql printsupport

LIBS += -lmosquitto

CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QMeteo
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qmosquitto.cpp \
    qweatherdata.cpp \
    dbmanager.cpp \
    entities.cpp \
    dialogstation.cpp \
    qcustomplot.cpp

HEADERS  += mainwindow.h \
    qmosquitto.h \
    qweatherdata.h \
    json.hpp \
    dbmanager.h \
    entities.h \
    dialogstation.h \
    qcustomplot.h

FORMS    += mainwindow.ui qweatherdata.ui \
    dialogstation.ui
