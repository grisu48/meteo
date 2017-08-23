#ifndef ENTITIES_H
#define ENTITIES_H

#include <QString>


/** Station entity */
class Station {
private:

public:
    long id = 0;
    QString name;
    QString desc;

    Station() {}
    Station(const long id, QString name = "", QString desc = "") {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }
    Station(const Station &station) {
        this->id = station.id;
        this->name = station.name;
        this->desc = station.desc;
    }
    virtual ~Station() {}
};


class DataPoint {
private:

public:
    long timestamp = 0L;
    float t = 0.0F;         // Temperature in deg. Celcius
    float hum = 0.0F;       // Humidity in % rel
    float p = 0.0F;         // Pressure in hPa
    float light = 0.0F;     // Light value (uncalibrated currently)

    DataPoint() {
        this->timestamp = 0L;
        this->t = 0.0F;
        this->hum = 0.0F;
        this->p = 0.0F;
        this->light = 0.0F;
    }
    DataPoint(const DataPoint &dp) {
        this->timestamp = dp.timestamp;
        this->t = dp.t;
        this->hum = dp.hum;
        this->p = dp.p;
        this->light = dp.light;
    }
};


#endif // ENTITIES_H
