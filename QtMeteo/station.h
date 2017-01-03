#ifndef STATION_H
#define STATION_H

#include <QString>
#include <QMap>
#include <QList>
#include <QVector>
#include <QTime>
#include <QDateTime>

class Station;
class Stations;


/** Station instance to keep track of a station's data */
class Station
{
private:
    /** Station id */
    long l_id;

    /** Data reference - First is the timestamp, then a map of actual data */
    QMap<long, QMap<QString, double> > data;

public:
    Station();
    Station(long id);

    long id(void) const { return this->l_id; }

    void push(QMap<QString, double> &data);

    void removeBefore(long timestamp);
    void removeBefore(QDateTime &time);

    /** Compact the data, i.e. reduce unnecessary ones */
    void compact(void);

    QMap<long, QMap<QString, double> > getData(void) const;
};


/** Station manager instance */
class Stations {
private:

    /** Known stations with data */
    QMap<long, Station> stations;

public:


    Stations();

    void push(long station, QMap<QString, double> &data);

    void clear(void);

    /** Get signleton instance */
    static Stations* instance(void) {
        static Stations *singleton;
        if(!singleton)
            singleton = new Stations();
        return singleton;
    }
};

#endif // STATION_H
