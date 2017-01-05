#ifndef STATION_H
#define STATION_H

#include <QString>
#include <QMap>
#include <QList>
#include <QVector>
#include <QTime>
#include <QDateTime>


#define S_ID_OUTDOOR 1
#define S_ID_LIVING 2
#define S_ID_FLEX 8



class Station;
class Stations;

/** Outdoor data */
struct {
    /** Timestamp in millis */
    long timestamp;
    /** Temperature in degree C */
    double t;
    /** Pressure in hPa*/
    double p;
    /** Humidity in rel %*/
    double h;
} typedef st_outdoor_t;

struct {
    /** Timestamp in millis */
    long timestamp;
    /** Temperature in degree C */
    double t;
    /** Humidity in rel %*/
    double h;

} typedef st_livin_t;

struct {
    /** Timestamp in millis */
    long timestamp;
    /** Temperature in degree C */
    double t;
    /** Humidity in rel %*/
    double h;

    /** Battery flag*/
    int battery;
} typedef st_flex_t;



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

#endif // STATION_H
