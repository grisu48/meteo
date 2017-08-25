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
};


#endif // ENTITIES_H
