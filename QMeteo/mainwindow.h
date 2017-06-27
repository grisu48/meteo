#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QInputDialog>
#include <QList>
#include <QMap>

#include "qmosquitto.h"
#include "qweatherdata.h"
#include "json.hpp"

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

private:
    Ui::MainWindow *ui;

    void connectMosquitto(const QString &remote, const int port = 1883);


    QList<QMosquitto*> mosquittos;
    QMap<long, QWeatherData*> stations;
};

#endif // MAINWINDOW_H
