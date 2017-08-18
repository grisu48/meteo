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


#endif // ENTITIES_H
