#ifndef QMETEO_H
#define QMETEO_H

#include <QObject>
#include <QEventLoop>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QList>
#include <QStringList>
#include <QDateTime>
#include <QTimer>

#include "entities.h"

/** QMeteo client */
class QMeteo : public QObject
{
    Q_OBJECT
private:
    /** Base link for remote */
    QString remote;

    QTimer *timer;

    long refreshDelay = 5000;

    long delta_millis = 0;    // Delta t in MILLISECONDS between local and remote
protected:
    QString fetch(const QString &link);

private slots:
    void timerCall();
public:
    QMeteo();
    virtual ~QMeteo();

    /** Fetch stations, block until received */
    QList<Station> stations();
    QList<DataPoint> currentReadings();

    /** Get the timestamp from the server and refresh the local delta */
    long timestamp();

    void setRemote(QString remote) { this->remote = remote; }
    void setRefreshDelay(const long millis) { this->refreshDelay = millis; }

    /** Start worker thread on this remote */
    void start();

    /**
     * @brief delta_milliseconds (local - remote) milliseconds difference. This is available after starting
     * @return milliseconds between local and remote
     */
    long delta_milliseconds(void) const { return this->delta_millis; }

signals:
    void onStationsFetched(const QList<Station> stations);
    void onDataUpdate(const QList<DataPoint> datatpoints);
};


#endif // QMETEO_H
