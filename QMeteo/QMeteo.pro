#-------------------------------------------------
#
# Project created by QtCreator 2017-08-25T10:05:44
#
#-------------------------------------------------

QT       += core gui printsupport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QMeteo
TEMPLATE = app
CONFIG += c++11


SOURCES += main.cpp\
        mainwindow.cpp \
    qmeteo.cpp \
    entities.cpp \
    qcustomplot.cpp \
    qweatherdata.cpp \
    dialogstation.cpp \
    config.cpp \
    dialogsettings.cpp

HEADERS  += mainwindow.h \
    qmeteo.h \
    entities.h \
    qcustomplot.h \
    qweatherdata.h \
    dialogstation.h \
    config.h \
    dialogsettings.h

FORMS    += mainwindow.ui \
    dialogstation.ui \
    qweatherdata.ui \
    dialogsettings.ui
