#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <string>

using nlohmann::json;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionConnect_triggered()
{
    bool ok = false;
    QString input = QInputDialog::getText(this, "Connect", "Remote host to connect to", QLineEdit::Normal, "localhost:1883", &ok);
    if(!ok) return;
    input = input.trimmed();
    if(input.isEmpty()) return;

    int port = 1883;
    QString remote = input;
    int i = remote.indexOf(':');
    if(i>=0) {
        port = remote.mid(i+1).toInt(&ok);
        if(!ok) {
            QMessageBox::warning(this, "Error", "Illegal port");
            return;
        }
        remote = remote.left(i);
    }

    this->connectMosquitto(remote, port);
}


void MainWindow::connectMosquitto(const QString &remote, const int port) {
    QMosquitto *mosquitto = NULL;
    try {
        mosquitto = new QMosquitto();
        mosquitto->connectTo(remote, port);

        this->connect(mosquitto, SIGNAL(onMessage(QString,QString)), this, SLOT(onMessage(QString,QString)));

        mosquitto->subscribe("meteo/#");
        mosquitto->start();

        this->mosquittos.append(mosquitto);
        ui->lblStatus->setText("Connected to host");

    } catch (const char* msg) {
        if(mosquitto != NULL) delete mosquitto;
        QString errmsg = "Error connecting to host " + remote + ":" + QString::number(port) + " - " + QString(msg);
        ui->lblStatus->setText(errmsg);
    } catch (...) {
        if(mosquitto != NULL) delete mosquitto;
        ui->lblStatus->setText("Unknown error connecting to host " + remote + ":" + QString::number(port));
    }
}

void MainWindow::onMessage(QString topic, QString message) {
    try {
        json j = json::parse(message.toStdString());

        const long id = j["node"].get<long>();
        QString name(j["name"].get<std::string>().c_str());

        const float p = j["p"].get<float>();
        const float t = j["t"].get<float>();
        const float hum = j["hum"].get<float>();
        //const float l_vis = j["l_vis"].get<float>();
        //const float l_ir = j["l_ir"].get<float>();

        QWeatherData *station = NULL;
        if(!this->stations.contains(id)) {
            station = new QWeatherData(ui->sclWidgets);
            ui->lyStations->addWidget(station);
            station->setName(name);
            this->stations[id] = station;
        } else
            station = this->stations[id];

        station->setTemperature(t);
        station->setPressure(p);
        station->setHumidity(hum);
        station->updateTimestamp();


        //ui->lblStatus->setText(QString::number(id) + " [" + name + "] - " + QString::number(t) + " deg C, " + QString::number(hum) + " % rel");
    } catch (std::exception &e) {
        ui->lblStatus->setText("Error: " + QString(e.what()));
    } catch (...) {
        ui->lblStatus->setText("Unknown error while receiving");
    }


}
