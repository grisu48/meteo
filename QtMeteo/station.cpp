#include "station.h"

static long getSystemTime(void) {
    return (long)QDateTime::currentMSecsSinceEpoch();
}


Station::Station()
{
    this->l_id = 0;
}

Station::Station(long id)
{
    this->l_id = id;
}

void Station::push(QMap<QString, double> &data) {
    const long millis = getSystemTime();
    this->data[millis] = data;
}

void Station::removeBefore(long timestamp) {
    for (auto it = this->data.begin(); it != this->data.end();) {
        if (it.key() < timestamp) it = this->data.erase(it);
        else ++it;
    }
}

void Station::removeBefore(QDateTime &time) {
    const long millis = getSystemTime();
    this->removeBefore(millis);
}


QMap<long, QMap<QString, double> > Station::getData(void) const {
    QMap<long, QMap<QString, double> > ret(this->data);
    return ret;
}

void Station::compact() {
    QList<long> timestamps = this->data.keys();
    if(timestamps.size() < 2) return;

    qSort(timestamps);
    const long min = timestamps[0];
    const long max = timestamps[timestamps.size()-1];


}




Stations::Stations() {

}


void Stations::push(long station, QMap<QString, double> &data) {
    if(!this->stations.contains(station))
        this->stations[station] = Station(station);
    this->stations[station].push(data);
    this->stations[station].compact();
}

void Stations::clear(void) {
    this->stations.clear();
}
