#ifndef METEO_TCPRECEIVER_H
#define METEO_TCPRECEIVER_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QMap>
#include <QTcpSocket>
#include <QDataStream>
#include <QDomElement>
#include <QDomDocument>

#include "weatherdata.h"

/** General receiver instance - Receives meteo data from multiple sources */
class TcpReceiver : public QObject
{
    Q_OBJECT

private:
    /** Receiver socket */
    QTcpSocket socket;

    /** Remote address */
    QString remote;
    /** Remote port */
    int port;

    /** Running flag */
    volatile bool running;

    /** Current packet */
    QString packet;

protected:

    /** Internal call to process a packet */
    void packetReceived(QString &packet);

public:
    TcpReceiver(const QString &hostName, quint16 port, QObject *parent = 0);
    virtual ~TcpReceiver();

    /** Close connection */
    void close();

    /**
     * @brief reconnect Connect or reconnect, if disconnected.
     * @return true if connected, false on error
     */
    bool reconnect(void);

signals:
    void onDataArrival(const WeatherData &data);
    void error(int socketError, const QString &message);
    void parseError(QString &message, QString &packet);

private slots:
    void onReadyRead();
};

#endif // TCPRECEIVER_H
