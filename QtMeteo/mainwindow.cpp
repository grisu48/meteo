#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>

static const QString configFilename = QDir::homePath() + QDir::separator() + ".qtmeteo.cf";

using std::cerr;
using std::cout;
using std::endl;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    qRegisterMetaType<QMap<QString,double> >("QMap<QString,double>");
    qRegisterMetaType<QString>("QString");
    qRegisterMetaType< WeatherData >("WeatherData");

    connect(&this->receiver, SIGNAL(dataArrival(WeatherData)), this, SLOT(receiver_newData(WeatherData)));


    config = new ConfigFile(configFilename);
    if(config->fileExists()) {
        if(config->contains("connect")) {
            QString remote = (*config)["connect"];
            this->connectTcp(remote, DEFAULT_PORT);
        }
        if(config->contains("listen")) {
            QString s_port = (*config)["listen"];
            bool ok;
            const int port = s_port.toInt(&ok);
            if(ok)
                this->listenUdp(port);
        }
    }
}

MainWindow::~MainWindow()
{
    this->closeConnection();
    delete config;
    delete ui;
}

void MainWindow::closeConnection(void) {
    this->receiver.close();
    ui->lblStatus->setText("Closed");
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::connectTcp(QString remote, int port) {
    try {
        ui->lblStatus->setText("Connecting to " + remote + ":" + QString::number(port) + " ... ");

        this->remote = remote;
        this->port = port;

        TcpReceiver *recv = NULL;
        try {
            recv = receiver.addTcpReceiver(remote, port);
            if(!recv->reconnect()) {
                receiver.removeReceiver(recv);
                ui->lblStatus->setText("Cannot connect to " + remote + ":" + QString::number(port));
                return;
            } else {
                // Start thread
                ui->lblStatus->setText("Connected");
            }
        } catch (...) {
            // Cleanup
            if(recv != NULL) {
                receiver.removeReceiver(recv);
            }
            throw;
        }

    } catch (const char* msg) {
        ui->lblStatus->setText("Error: " + QString(msg));
    }
}

void MainWindow::listenUdp(const int port) {
    try {
        UdpReceiver *recv = receiver.addUdpReceiver(port);
        if(recv == NULL) return;
        ui->lblStatus->setText("UDP receiver bound: " + QString::number(port));
    } catch (const char* msg) {
        ui->lblStatus->setText("Error: " + QString(msg));
    }
}

void MainWindow::on_actionConnect_triggered()
{
    QInputDialog qInput(this);
    qInput.setLabelText("Remote server");
    QString text = "";
    if(!remote.isEmpty()) {
       text = remote;
       if(port > 0) text += ":" + QString::number(port);
    }
    qInput.setTextValue(text);
    qInput.setCancelButtonText("Cancel");
    qInput.setOkButtonText("Connect");
    qInput.setToolTip("REMOTE[:PORT]");
    qInput.show();
    qInput.exec();
    QString remote = qInput.textValue();
    if(remote.isEmpty()) return;

    try {
        int port = DEFAULT_PORT;
        if(remote.contains(":")) {
            const int pos = remote.indexOf(':');
            QString sPort = remote.mid(pos+1);
            bool ok;
            port = sPort.toInt(&ok);
            if(!ok) throw "Illegal port";
            remote = remote.left(pos);
            if(remote.isEmpty()) throw "No remote given";

        }
        this->connectTcp(remote, port);


    } catch (const char* msg) {
        ui->lblStatus->setText("Error: " + QString(msg));
    }


}

void MainWindow::receiver_newData(const WeatherData &data) {
    QDateTime now = QDateTime::currentDateTime();

    const long station = data.station();
    ui->lblStatus->setText("New data from station " + QString::number(station) + " [" + now.toString("HH:mm:ss") + "]");

    QWeatherData *widget = NULL;
    if(w_widgets.contains(station)) {
        widget = w_widgets[station];
    } else {
        widget = new QWeatherData(ui->scrollSensors);
        w_widgets[station] = widget;
        ui->lySensors->addWidget(widget);

        QString name = "Station " + QString::number(station);
        QString key = "station_" +  QString::number(station);
        if(config->contains(key))
            name = (*config)[key];

        if(station == S_ID_FLEX) {
            widget->setTemperatureRange(10,30);
            widget->setHumidity(20,60);
        } else if(station == S_ID_OUTDOOR) {
            widget->setTemperatureRange(-20,40);
            widget->setHumidity(0,100);
        } else if(station == S_ID_LIVING) {
            widget->setTemperatureRange(10,30);
            widget->setHumidity(20,60);
        }

        widget->setName(name);
    }



    // Refresh data
    widget->setTemperature(data.temperature());
    widget->setHumidity(data.humidity());
    widget->setPressure(data.pressure());
    widget->setLight(data.light());
    widget->setStatus("Update: [" + now.toString("HH:mm:ss") + "]");
}

void MainWindow::receiver_error(int socketError, const QString &message) {
    ui->lblStatus->setText("Socket error " + QString::number(socketError) + ": " + message);
}

void MainWindow::receiver_parseError(QString &message, QString &packet) {
    ui->lblStatus->setText("Parse error: " + message);
    cerr << packet.toStdString() << endl;
}

void MainWindow::on_actionClose_triggered()
{
    this->closeConnection();
}

void MainWindow::on_actionListen_triggered()
{
    QInputDialog qInput(this);
    qInput.setLabelText("Local UDP port");
    qInput.setTextValue("5232");
    qInput.setCancelButtonText("Cancel");
    qInput.setOkButtonText("Connect");
    qInput.setToolTip("PORT");
    qInput.show();
    qInput.exec();
    QString remote = qInput.textValue().trimmed();
    if(remote.isEmpty()) return;

    try {
        bool ok;
        int port = remote.toInt(&ok);
        if(ok)
            this->listenUdp(port);

    } catch (const char* msg) {
        ui->lblStatus->setText("Error: " + QString(msg));
    }

}

void MainWindow::on_actionClear_triggered()
{
    foreach(QWeatherData *widget, w_widgets.values()) {
        ui->lySensors->removeWidget(widget);
        delete widget;
    }
    w_widgets.clear();
}

void MainWindow::on_actionName_stations_triggered()
{
    // XXX This is still nasty, but it works :-)

    bool ok;
    QString input;

    input = QInputDialog::getText(this, "Station", "Station ID", QLineEdit::Normal,"", &ok).trimmed();
    if(!ok || input.isEmpty()) return;
    const int id = input.toInt(&ok);
    if(!ok) return;

    input = QInputDialog::getText(this, "Station", "Give the name for station " + QString::number(id), QLineEdit::Normal,"", &ok).trimmed();
    if(!ok || input.isEmpty()) return;

    QString name = input;
    QString key = "station_" + QString::number(id);

    this->config->putValue(key, name);
}

void MainWindow::on_actionWrite_settings_to_file_triggered()
{
    if(!this->remote.isEmpty()) {
        QString remote = this->remote;
        this->config->putValue("connect", remote);
    }
    this->config->write(configFilename);
    ui->lblStatus->setText("Configuration written to file");
}
