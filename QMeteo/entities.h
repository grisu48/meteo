#ifndef ENTITIES_H
#define ENTITIES_H

#include <QString>

class Station {
public:
    long id;
    QString name;
    Station() {
        this->id = 0L;
        this->name = "";
    }
    Station(const Station &src) {
        this->id = src.id;
        this->name = src.name;
    }
};

class DataPoint {
public:
    long station;
    /** Timestamp in SECONDS since EPOC */
    long timestamp;
    float t;
    float hum;
    float p;
    float l_ir;
    float l_vis;

    DataPoint() {
        this->station = 0L;
        this->timestamp = 0L;
        this->t = 0;
        this->hum = 0;
        this->p = 0;
        this->l_ir = 0;
        this->l_vis = 0;
    }

    DataPoint(const DataPoint &src) {
        this->station = src.station;
        this->timestamp = src.timestamp;
        this->t = src.t;
        this->hum = src.hum;
        this->p = src.p;
        this->l_ir = src.l_ir;
        this->l_vis = src.l_vis;
    }

    DataPoint &operator=(const DataPoint &src) {
        this->station = src.station;
        this->timestamp = src.timestamp;
        this->t = src.t;
        this->hum = src.hum;
        this->p = src.p;
        this->l_ir = src.l_ir;
        this->l_vis = src.l_vis;
        return *this;
    }
};


/** Lightning event */
class Lightning {
public:
    /** Timestamp in seconds since EPOC */
    long timestamp;
    /** Distance from station in km */
    float distance;
    /** Station id */
    long station;

    Lightning() {
        this->station = 0L;
        this->timestamp = 0L;
        this->distance = 0.0F;
    }

    Lightning(const Lightning &src) {
        this->station = src.station;
        this->timestamp = src.timestamp;
        this->distance = src.distance;
    }

    Lightning& operator=(const Lightning &src) {
        this->station = src.station;
        this->timestamp = src.timestamp;
        this->distance = src.distance;
        return *this;
    }

    QString toString() const {
        QString ret;
        ret = "Lightning (" + QString::number(this->timestamp) + ") [Station " + QString::number(this->station) + "] " + QString::number(this->distance) + " km";
        return ret;
    }

    bool operator!=(const Lightning &src) const {
        if(src.timestamp != this->timestamp) return true;
        if(src.station != this->station) return true;
        if(src.distance != this->distance) return true;
        return false;
    }

    bool operator==(const Lightning &src) const {
        if(src.timestamp != this->timestamp) return false;
        if(src.station != this->station) return false;
        if(src.distance != this->distance) return false;
        return true;
    }
};


#endif // ENTITIES_H
