#ifndef RECEIVERTHREAD_H
#define RECEIVERTHREAD_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QMap>
#include <QTcpSocket>
#include <QDataStream>
#include <QDomElement>
#include <QDomDocument>

class ReceiverThread : public QThread
{
    Q_OBJECT

private:
    QTcpSocket socket;

    volatile bool running = true;


    void packetReceived(QString &packet);
public:
    explicit ReceiverThread(const QString &hostName, quint16 port, QObject *parent = 0);
    virtual ~ReceiverThread();

    void run() Q_DECL_OVERRIDE;
    void close();

signals:
    void newData(const long station, QMap<QString, double> values);
    void error(int socketError, const QString &message);
    void parseError(QString &message, QString &packet);

public slots:
};

#endif // RECEIVERTHREAD_H
