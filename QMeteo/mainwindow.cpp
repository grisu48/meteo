#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
    if(this->meteo != NULL) delete this->meteo;
}

void MainWindow::on_actionConnect_triggered()
{
    if(this->meteo != NULL) {
        this->on_actionDisconnect_triggered();
    }
    this->meteo = new QMeteo();
    connect(this->meteo, SIGNAL(onDataUpdate(QList<DataPoint>)), this, SLOT(onDataReceived(QList<DataPoint>)));

    meteo->setRemote("localhost:8900");
    meteo->setRefreshDelay(1000);
    QList<Station> stations = meteo->stations();
    QString text;
    foreach(const Station &station, stations) {
        text += QString::number(station.id) + "   " + station.name + "\n";
    }

    meteo->start();
    text += "Delta t = " + QString::number(meteo->delta_milliseconds()) + " ms";

    ui->txtLog->setPlainText(text);
}

void MainWindow::on_actionDisconnect_triggered()
{
    if(this->meteo != NULL) {
        disconnect(this->meteo, SIGNAL(onDataUpdate(QList<DataPoint>)), this, SLOT(onDataReceived(QList<DataPoint>)));
        delete this->meteo;
        this->meteo = NULL;
    }
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::exit(0);
}

void MainWindow::onDataReceived(const QList<DataPoint> datapoints) {
    QString text;
    foreach(const DataPoint &dp, datapoints) {
        text += QString::number(dp.station) + "   " + QString::number(dp.t) + " deg C, " + QString::number(dp.hum) + " % rel, " + QString::number(dp.p) + " hPa";
    }
    ui->txtLog->setPlainText(text);
}
