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
private:
    QSqlDatabase m_db;

protected:
    bool exec(QString sql);

};

#endif // DBMANAGER_H
