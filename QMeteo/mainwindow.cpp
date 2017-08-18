#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <string>

using nlohmann::json;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    QList<Station> stations = db_manager->stations();
    for(int i=0;i<stations.size();i++) {
        Station station = stations[i];
        QWeatherData *widget = this->station(station.id);
        widget->setName(station.name);
    }
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
        mosquitto->subscribe("lightning/#");
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

QWeatherData* MainWindow::station(const long id) {
    QWeatherData *station = NULL;

    if(!this->stations.contains(id)) {
        station = new QWeatherData(ui->sclWidgets);
        ui->lyStations->addWidget(station);
        this->stations[id] = station;

        // Insert station into db, if not existing
        Station objStation(id, "", "");
        db_manager->insertStation(objStation, true);
    } else
        station = this->stations[id];
    return station;
}

void MainWindow::onMessage(QString topic, QString message) {
    // Check if meteo or lightning data
    if(topic.startsWith("lightning/")) {

        // Determine node id
        long id;
        bool ok;
        QString temp = topic.mid(10);        // remove "lightning/" at beginning
        id = temp.toLong(&ok);
        if(!ok || id < 0) return;

        int i = message.indexOf('\t');
        QString strDate = message.left(i).trimmed();
        message = message.mid(i+1).trimmed();


        QWeatherData *station = this->station(id);

        if(message == "DISTURBER DETECTED") {
            station->setLightningDisturberDetected();
        } else if(message == "NOISE DETECTED") {
            station->setLightningNoiseDetected();
        } else {
            ui->lblStatus->setText(strDate + " - " + message);
        }

    } else if(topic.endsWith("/lightning")) {
        // This is an old protocol and is not supported anymore
        return;
    } else {
        // Assume meteo data
        try {
            json j = json::parse(message.toStdString());

            const long id = j["node"].get<long>();
            QString name(j["name"].get<std::string>().c_str());

            const float p = j["p"].get<float>();
            const float t = j["t"].get<float>();
            const float hum = j["hum"].get<float>();
            //const float l_vis = j["l_vis"].get<float>();
            //const float l_ir = j["l_ir"].get<float>();

            QWeatherData *station = this->station(id);
            station->setName(name);

            station->setName(name);
            station->setTemperature(t);
            station->setPressure(p);
            station->setHumidity(hum);
            station->updateTimestamp();

            const long timestamp = (QDateTime::currentMSecsSinceEpoch()/1000L);
            db_manager->insertDatapoint(id, timestamp, t, hum, p, 0.0F);

            //ui->lblStatus->setText(QString::number(id) + " [" + name + "] - " + QString::number(t) + " deg C, " + QString::number(hum) + " % rel");
        } catch (std::exception &e) {
            ui->lblStatus->setText("Error: " + QString(e.what()));
        } catch (...) {
            ui->lblStatus->setText("Unknown error while receiving");
        }
    }

}
