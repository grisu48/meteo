#include "dbmanager.h"

DbManager::DbManager(const QString& path)
{
   this->m_db = QSqlDatabase::addDatabase("QSQLITE");
   this->m_db.setDatabaseName(path);

   if (!this->m_db.open()) {
      throw "Error connecting to database";
   }

   if(!exec("CREATE TABLE IF NOT EXISTS `stations` (`id` INT PRIMARY KEY, `name` TEXT, `desc` TEXT);")) throw "Error creating stations table";
}


void DbManager::insertStation(const Station &station, const bool ignore) {
    QString sql = QString("INSERT OR ") + (ignore?"IGNORE":"REPLACE") + " INTO `stations` (`id`,`name`,`desc`) VALUES (?, ?, ?)";
    QSqlQuery query(sql, this->m_db);
    query.prepare(sql);
    query.bindValue(0, QString::number(station.id), QSql::ParamTypeFlag::In);
    query.bindValue(1, station.name, QSql::ParamTypeFlag::In);
    query.bindValue(2, station.desc, QSql::ParamTypeFlag::In);
    if(!query.exec()) throw "Error inserting values";

    exec("CREATE TABLE IF NOT EXISTS `station_" + QString::number(station.id) + "` (`timestamp` INT PRIMARY KEY, `temperature` REAL, `humidity` REAL, `pressure` REAL, `light` REAL);");
}

void DbManager::insertDatapoint(const long station, const long timestamp, const float temperature, const float humidity, const float pressure, const float light) {
    QString sql = "INSERT INTO `station_" + QString::number(station) + "` (`timestamp`,`temperature`,`humidity`, `pressure`, `light`) VALUES (?, ?, ?, ?, ?)";
    QSqlQuery query(sql, this->m_db);
    query.prepare(sql);
    query.bindValue(0, QString::number(timestamp));
    query.bindValue(1, temperature);
    query.bindValue(2, humidity);
    query.bindValue(3, pressure);
    query.bindValue(4, light);
    if(!query.exec()) throw "Error inserting values";
}

bool DbManager::exec(QString sql) {
    QSqlQuery query(sql, this->m_db);
    return query.exec();
}

QList<Station> DbManager::stations() {
    QList<Station> stations;

    QString sql = "SELECT `id`,`name`,`desc` FROM `stations`;";
    QSqlQuery query(sql, this->m_db);
    if(!query.exec()) throw "Error querying stations";
    while(query.next()) {
        long id = (long)query.value(0).toLongLong();
        QString name = query.value(1).toString();
        QString desc = query.value(2).toString();

        Station station(id, name, desc);
        stations.push_back(station);
    }

    return stations;
}
