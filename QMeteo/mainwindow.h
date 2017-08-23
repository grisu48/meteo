#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QInputDialog>
#include <QString>
#include <QList>
#include <QMap>
#include <QDir>

#include "qmosquitto.h"
#include "qweatherdata.h"
#include "json.hpp"
#include "entities.h"
#include "dbmanager.h"
#include "dialogstation.h"

namespace Ui {

class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionConnect_triggered();

protected slots:

    void onMessage(QString topic, QString message);
    void onWeaterStationLinkClicked(const QString &link, const long stationId);

private:
    Ui::MainWindow *ui;

    void connectMosquitto(const QString &remote, const int port = 1883);

    QWeatherData* station(const long id);


    QList<QMosquitto*> mosquittos;
    QMap<long, QWeatherData*> stations;

    QString dbFilename = QDir::homePath() + QDir::separator() + ".qmeteo.db";
    DbManager *db_manager = new DbManager(dbFilename);
};

#endif // MAINWINDOW_H
