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
    bool ok = false;
    QString input = QInputDialog::getText(this, "Connect", "Remote host to connect to", QLineEdit::Normal, "localhost:8900", &ok);
    if(!ok) return;
    input = input.trimmed();
    if(input.isEmpty()) return;


    if(this->meteo != NULL) {
        this->on_actionDisconnect_triggered();
    }
    this->meteo = new QMeteo();
    connect(this->meteo, SIGNAL(onDataUpdate(QList<DataPoint>)), this, SLOT(onDataReceived(QList<DataPoint>)));

    meteo->setRemote(input);
    meteo->setRefreshDelay(1000);
    QList<Station> stations = meteo->stations();
    foreach(const Station &station, stations) {
        this->station(station.id)->setName(station.name);
    }

    meteo->start();
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
        QWeatherData *station = this->station(dp.station);

        station->setTemperature(dp.t);
        station->setHumidity(dp.hum);
        station->setPressure(dp.p);
        station->setLight((dp.l_ir + dp.l_vis)/2.0F);
    }
}


QWeatherData* MainWindow::station(const long id) {
    QWeatherData *station = NULL;
    if(!this->stations.contains(id)) {
        station = new QWeatherData(ui->scMeteo);
        ui->lyStations->addWidget(station);
        this->stations[id] = station;
    } else
        station = this->stations[id];
    return station;
}

void MainWindow::clear() {
    foreach(QWeatherData *station, this->stations)
        ui->lyStations->removeWidget(station);
    foreach(QWeatherData *station, this->stations)
        delete station;
    this->stations.clear();
}
