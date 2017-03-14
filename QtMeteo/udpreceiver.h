#ifndef METEO_UDPRECEIVER_H
#define METEO_UDPRECEIVER_H

#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QDomElement>
#include <QDomDocument>

#include "weatherdata.h"

class UdpReceiver : public QObject
{
    Q_OBJECT

protected:
    QUdpSocket socket;

    void processDatagram(QString packet, const QHostAddress &host, const int port);
public:
    explicit UdpReceiver(quint16 port, QObject *parent = 0);
    virtual ~UdpReceiver();

    void close(void);

signals:
    void onDataArrival(const WeatherData &data);
    void error(int socketError, const QString &message);
    void parseError(QString &message, QString &packet);


private slots:
    void onDatagramArrival();

};

#endif // UDPRECEIVER_H
