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
    // TODO: Implement me

}




Stations::Stations() {

}


static double getData(const QMap<QString, double> &data, const QString keyword, const double defaultValue = 0.0) {
    if(data.contains(keyword)) return data[keyword];
    else return defaultValue;
}

void Stations::push(const long station, const QMap<QString, double> &data) {
#if 0
    if(!this->stations.contains(station))
        this->stations[station] = Station(station);
    this->stations[station].push(data);
    this->stations[station].compact();
#endif

    const long millis = getSystemTime();

    if(station == S_ID_OUTDOOR) {

        st_outdoor_t tt;
        tt.timestamp = millis;
        tt.t = getData(data, "temperature");
        tt.p = getData(data, "pressure");
        tt.h = getData(data, "humidity");

        this->lOutdoor.push_back(tt);

    } else if(station == S_ID_LIVING) {

        st_livin_t tt;
        tt.timestamp = millis;
        tt.t = getData(data, "temperature");
        tt.h = getData(data, "humidity");

        this->lLivingRoom.push_back(tt);

    } else if(station == S_ID_FLEX) {

        st_flex_t tt;
        tt.timestamp = millis;
        tt.t = getData(data, "temperature");
        tt.h = getData(data, "humidity");
        tt.battery = (int)getData(data, "battery");

        this->lFlex.push_back(tt);

    }
}

void Stations::clear(void) {
    this->stations.clear();
    this->lFlex.clear();
    this->lLivingRoom.clear();
    this->lOutdoor.clear();
}
