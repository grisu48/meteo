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

    // Try to connect to patatoe.wg
    this->connectStation("patatoe.wg", 8888);

}

MainWindow::~MainWindow()
{
    this->closeConnection();
    delete ui;
}

void MainWindow::closeConnection(void) {
    if(this->receiver == NULL) return;
    else {
        this->receiver->close();
        delete this->receiver;
        this->receiver = NULL;
    }
    ui->lblStatus->setText("Closed");
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::connectStation(QString remote, int port) {
    this->closeConnection();

    try {
        ui->lblStatus->setText("Connecting to " + remote + ":" + QString::number(port) + " ... ");

        this->remote = remote;
        this->port = port;

        ReceiverThread *recv = NULL;
        try {
            recv = new ReceiverThread(remote, port, this);

            connect(recv, SIGNAL(error(int,QString)), this, SLOT(receiver_error(int,QString)));
            connect(recv, SIGNAL(newData(const long,QMap<QString,double>)), this, SLOT(receiver_newData(const long,QMap<QString,double>)));
            connect(recv, SIGNAL(parseError(QString&,QString&)), this, SLOT(receiver_parseError(QString&,QString&)));
            // Start thread
            ui->lblStatus->setText("Connected");
            recv->start();
            recv->queryNodes();
            this->receiver = recv;
        } catch (...) {
            // Cleanup
            if(recv != NULL) delete recv;
            throw;
        }



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
        int port = 8888;
        if(remote.contains(":")) {
            const int pos = remote.indexOf(':');
            QString sPort = remote.mid(pos+1);
            bool ok;
            port = sPort.toInt(&ok);
            if(!ok) throw "Illegal port";
            remote = remote.left(pos);
            if(remote.isEmpty()) throw "No remote given";

        }
        this->connectStation(remote, port);


    } catch (const char* msg) {
        ui->lblStatus->setText("Error: " + QString(msg));
    }


}

void MainWindow::receiver_newData(const long station, QMap<QString, double> values) {
    ui->lblStatus->setText("New data from station " + QString::number(station) + ".");


    if(station == 8) {      // Living room
        cout << "ROOM 8";
        if(values.contains("temperature")) {
            const double temperature = values["temperature"];
            ui->txtLivingRoomTemperature->setText(QString::number(temperature) + " degree C");
            cout << " temperature = " << temperature;
        }
        if(values.contains("humidity")) {
            const double hum = values["humidity"];
            ui->txtLivingRoomHumidity->setText(QString::number(hum) + " % relative");
            cout << " humidity = " << hum;
        }
        if(values.contains("light")) {
            const float light = (float)values["light"];
            ui->txtLivingRoomLight->setText(QString::number(light) + " / 255");
            cout << " light = " << light;
        }

        cout << endl;

        QDateTime now = QDateTime::currentDateTime();
        ui->lblLivingRoomRefreshed->setText("Last refreshed: " + now.toString());
    }
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
