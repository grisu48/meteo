#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include "entities.h"

class DbManager
{
public:
    DbManager(const QString& path);

    void insertStation(const Station &station, const bool ignore = true);
    void insertDatapoint(const long station, const long timestamp, const float temperature, const float humidity, const float pressure, const float light);

    QList<Station> stations();
    Station station(const long id, bool *ok = NULL);

    /**
     * @brief getDatapoints Fetch datapoints
     * @param station Station id
     * @param minTimestamp Lower time bounds in seconds or -1
     * @param maxTimestamp High time bound in seconds or -1
     * @param limit Maximum number of items to fetch
     * @return
     */
    QList<DataPoint> getDatapoints(const long station, const long minTimestamp = -1L, const long maxTimestamp = -1L, const long limit = 100);
private:
    QSqlDatabase m_db;

protected:
    bool exec(QString sql);

};

#endif // DBMANAGER_H
