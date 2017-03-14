#ifndef RECEIVER_H
#define RECEIVER_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QMap>
#include <QList>
#include <QTcpSocket>
#include <QDataStream>
#include <QDomElement>
#include <QDomDocument>

#include "tcpreceiver.h"
#include "udpreceiver.h"
#include "weatherdata.h"

/** General receiver instance - Receives meteo data from multiple sources */
class Receiver : public QObject
{
    Q_OBJECT

private:
    /** Attached tcp receivers */
    QList<TcpReceiver*> tcpReceivers;

    /** Attached udp receivers */
    QList<UdpReceiver*> udpReceivers;


public:
    Receiver();
    virtual ~Receiver();

    /** Close all sockets */
    void close(void);

    /** Adds a TCP receiver */
    TcpReceiver* addTcpReceiver(const QString &remoteHost, const int port);

    /** Adds a UDP receiver */
    UdpReceiver* addUdpReceiver(const int port);

    void removeReceiver(TcpReceiver *receiver);

signals:
    /** Signal when new data arrives */
    void dataArrival(const WeatherData &data);

private slots:

    void onDataArrival(const WeatherData &data);
};

#endif // RECEIVER_H
