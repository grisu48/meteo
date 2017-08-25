#-------------------------------------------------
#
# Project created by QtCreator 2017-08-25T10:05:44
#
#-------------------------------------------------

QT       += core gui printsupport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QMeteo
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qmeteo.cpp \
    entities.cpp \
    qcustomplot.cpp

HEADERS  += mainwindow.h \
    qmeteo.h \
    entities.h \
    qcustomplot.h

FORMS    += mainwindow.ui
