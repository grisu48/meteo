#include "qmeteo.h"

#include <iostream>
using namespace std;

QMeteo::QMeteo()
{
    this->timer = new QTimer(this);
    connect(this->timer, SIGNAL(timeout()), this, SLOT(timerCall()));
}

QMeteo::~QMeteo() {
    delete this->timer;
}


QString QMeteo::fetch(const QString &link) {
    // cout << link.toStdString() << endl;


    // create custom temporary event loop on stack
    QEventLoop eventLoop;

    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    // the HTTP request
    QNetworkRequest req;
    req.setUrl(QUrl(link));
    QNetworkReply *reply = mgr.get(req);
    eventLoop.exec(); // blocks stack until "finished()" has been called

    if (reply->error() == QNetworkReply::NoError) {
        QString data(reply->readAll());
        return data;
    } else {
        throw "Network error";
    }
}

QList<Station> QMeteo::stations() {
    QList<Station> ret;

    QString link = "http://" + this->remote + "/nodes?format=plain";
    QString data = fetch(link);
    QStringList list = data.split("\n");
    foreach(const QString line, list) {
        QString tmp = line.trimmed();
        if(tmp.isEmpty()) continue;
        QStringList splitted = tmp.split(",");
        if(splitted.size() < 2) continue;

        Station station;
        station.id = splitted[0].toLong();
        station.name = splitted[1];

        ret.append(station);
    }
    return ret;
}

QList<DataPoint> QMeteo::currentReadings() {
    QList<DataPoint> ret;

    QString link = "http://" + this->remote + "/current?format=plain";
    QString data = fetch(link);

    QStringList list = data.split("\n");
    foreach(const QString line, list) {
        QString tmp = line.trimmed();
        if(tmp.isEmpty()) continue;
        QStringList splitted = tmp.split(",");
        if(splitted.size() != 6) continue;

        DataPoint dp;

        dp.station = splitted[0].toLong();
        dp.timestamp = QDateTime::currentMSecsSinceEpoch()/1000L;
        dp.t = splitted[2].toFloat();
        dp.hum = splitted[3].toFloat();
        dp.p = splitted[4].toFloat();
        splitted = splitted[5].split("/");
        if(splitted.size() == 2) {
            dp.l_ir = splitted[0].toFloat();
            dp.l_vis = splitted[1].toFloat();
        }

        ret.append(dp);
    }

    return ret;
}

long QMeteo::timestamp() {
    QString link = "http://" + this->remote + "/timestamp?format=plain";
    QString data = fetch(link);
    data = data.trimmed();
    QStringList splitted = data.split(",");
    long timestamp = splitted[0].toLong();

    this->delta_millis = QDateTime::currentMSecsSinceEpoch() - timestamp;
    return timestamp;
}

QList<DataPoint> QMeteo::query(long station, long minTimestamp, long maxTimestamp, long limit) {
    QString link = "http://" + this->remote + "/node?id=" + QString::number(station) + "&format=plain&t_min=" + QString::number(minTimestamp) + "&t_max=" + QString::number(maxTimestamp) + "&limit=" + QString::number(limit);
    QString data = fetch(link);
    data = data.trimmed();
    QStringList lines = data.split("\n");

    QList<DataPoint> ret;
    foreach(QString line, lines) {
        line = line.trimmed();
        if(line.isEmpty() || line.at(0) == '#') continue;

        QStringList splitted = line.split(",");
        if(splitted.size() < 5) continue;

        DataPoint dp;

        dp.timestamp = splitted[0].toLong();
        dp.t = splitted[1].toFloat();
        dp.hum = splitted[2].toFloat();
        dp.p = splitted[3].toFloat();
        splitted = splitted[4].split("/");
        if(splitted.size() > 1) {
            dp.l_ir = splitted[0].toFloat();
            dp.l_vis = splitted[1].toFloat();
        }
        ret.push_back(dp);
    }
    return ret;
}

QList<Lightning> QMeteo::queryLightnings(long minTimestamp, long maxTimestamp, long limit) {
    QString link = "http://" + this->remote + "/lightnings?format=plain";
    if(minTimestamp >= 0) link += "&t_min=" + QString::number(minTimestamp);
    if(maxTimestamp >= 0) link += "&t_max=" + QString::number(maxTimestamp);
    if(limit > 0) link += "&limit=" + QString::number(limit);
    QString data = fetch(link);
    data = data.trimmed();
    QStringList lines = data.split("\n");

    QList<Lightning> ret;
    foreach(QString line, lines) {
        line = line.trimmed();
        if(line.isEmpty() || line.at(0) == '#') continue;

        QStringList splitted = line.split(",");
        if(splitted.size() < 3) continue;

        Lightning dp;

        dp.timestamp = splitted[0].toLong();
        dp.station = splitted[1].toLong();
        dp.distance = splitted[2].toFloat();
        ret.push_back(dp);
    }
    return ret;

}

void QMeteo::start() {
    // Evaluate delta T
    this->timestamp();

    this->timer->setInterval(this->refreshDelay);
    this->timer->start(this->refreshDelay);
}



void QMeteo::timerCall() {
    try {
        QList<DataPoint> readings = currentReadings();
        emit onDataUpdate(readings);
        QList<Lightning> lightnings = queryLightnings(-1,-1,10);
        emit onLightningsUpdate(lightnings);
    } catch (...) {
        // Ignore
    }
}
