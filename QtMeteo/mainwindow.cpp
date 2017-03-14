#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>


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

    // Try to connect to patatoe.wg
    this->connectTcp("patatoe.wg", DEFAULT_PORT);
    this->listenUdp();
}

MainWindow::~MainWindow()
{
    this->closeConnection();
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

        QString name;
        if(station == S_ID_FLEX) {
            name = "Flex' room";
            widget->setTemperatureRange(10,30);
            widget->setHumidity(20,60);
        } else if(station == S_ID_OUTDOOR) {
            name = "Outdoors";
            widget->setTemperatureRange(-20,40);
            widget->setHumidity(0,100);
        } else if(station == S_ID_LIVING) {
            name = "Living room";
            widget->setTemperatureRange(10,30);
            widget->setHumidity(20,60);
        } else {
            name = "Station " + QString::number(station);
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

void MainWindow::on_actionReconnect_triggered()
{
    this->closeConnection();
    this->connectTcp(this->remote, this->port);
}
