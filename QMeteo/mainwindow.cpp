#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Load default settings
    Config config(QDir::homePath() + "/.qmeteo.cf");
    this->remoteHost = config.get("remoteHost", this->remoteHost);

    long l_interval = 0L;
    QString interval = config.get("refreshInterval", "").trimmed();
    if(!interval.isEmpty()) {
        bool ok = false;
        l_interval = interval.toLong(&ok);
        if(ok)
            this->refreshInterval = l_interval;
    }

    if(config.get("autoconnect", "0") == "1")
        this->connectRemote(this->remoteHost);
}

MainWindow::~MainWindow()
{
    delete ui;
    if(this->meteo != NULL) delete this->meteo;
}

void MainWindow::on_actionConnect_triggered()
{
    bool ok = false;
    QString input = QInputDialog::getText(this, "Connect", "Remote host to connect to", QLineEdit::Normal, remoteHost, &ok);
    if(!ok) return;
    input = input.trimmed();
    if(input.isEmpty()) return;

    this->connectRemote(input);
}


void MainWindow::connectRemote(const QString remote) {
    if(this->meteo != NULL) {
        this->on_actionDisconnect_triggered();
    }


    this->remoteHost = remote;
    this->meteo = new QMeteo();
    if(this->refreshInterval > 0)
        this->meteo->setRefreshDelay(this->refreshInterval);
    try {
        this->clear();
        connect(this->meteo, SIGNAL(onDataUpdate(QList<DataPoint>)), this, SLOT(onDataReceived(QList<DataPoint>)));
        connect(this->meteo, SIGNAL(onLightningsUpdate(QList<Lightning>)), this, SLOT(onLightningsReceived(QList<Lightning>)));

        meteo->setRemote(this->remoteHost);
        QList<Station> stations = meteo->stations();
        foreach(const Station &station, stations) {
            this->station(station.id)->setName(station.name);
        }

        meteo->start();
        ui->lblStatus->setText("Status: Connected");

        this->on_btnFetchLightningToday_clicked();
    } catch (const char* msg) {
        if(meteo != NULL) delete meteo;
        meteo = NULL;
        ui->lblStatus->setText("Status: " + QString(msg));
    } catch(...) {
        if(meteo != NULL) delete meteo;
        meteo = NULL;
        ui->lblStatus->setText("Unknown error occurred");
    }
}

void MainWindow::on_actionDisconnect_triggered()
{
    if(this->meteo != NULL) {
        disconnect(this->meteo, SIGNAL(onDataUpdate(QList<DataPoint>)), this, SLOT(onDataReceived(QList<DataPoint>)));
        delete this->meteo;
        this->meteo = NULL;
    }
    ui->lblStatus->setText("Status: Disconnected");
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

void MainWindow::onLightningsReceived(const QList<Lightning> lightnings) {
    foreach(const Lightning &lightning, lightnings) {
        if(this->lightnings.contains(lightning))
            continue;
        else {
            QDateTime time;
            time.setMSecsSinceEpoch(lightning.timestamp*1000L);
            QString text = "[Station " + QString::number(lightning.station) + "] " + time.toString() + " in " + QString::number(lightning.distance) + " km";
            ui->lstLightnings->addItem(text);
            this->lightnings.append(lightning);
        }
    }
    QDateTime now = QDateTime::currentDateTime();
    ui->lblLightningStatus->setText("Last refresh: " + now.toString());

}

QWeatherData* MainWindow::station(const long id) {
    QWeatherData *station = NULL;
    if(!this->stations.contains(id)) {
        station = new QWeatherData(ui->scMeteo);
        station->stationId(id);
        connect(station, SIGNAL(onLinkClicked(QString,long)), this, SLOT(onStationClicked(QString,long)));
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

void MainWindow::onStationClicked(QString link, const long station) {
    (void)link;

    DialogStation *dialog = new DialogStation(station, this->meteo->getRemote());
    dialog->show();
    dialog->exec();
}

void MainWindow::on_btnFetchLightningToday_clicked()
{
    if(this->meteo == NULL) return;

    QDateTime today = QDateTime::currentDateTime();
    today.setTime(QTime(0,0,0,0));

    const long t_min = today.toMSecsSinceEpoch()/1000L;
    const long t_max = t_min + 60*60*24L;
    QList<Lightning> lightnings = this->meteo->queryLightnings(t_min, t_max);
    this->onLightningsReceived(lightnings);
}

void MainWindow::on_actionSettings_triggered()
{
    DialogSettings *dialog = new DialogSettings(this);
    dialog->show();
    dialog->exec();
}
