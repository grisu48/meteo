#ifndef MOSQUITTO_H
#define MOSQUITTO_H

#include <QObject>

#include <mosquitto.h>

class QMosquitto : public QObject
{
    Q_OBJECT
private:
    mosquitto *mosq = NULL;

    volatile bool running = false;

protected:
    /** Called when a message is received. Default behaviour: emit signal */
    virtual void onReceived(QString topic, QString message);

    // This call can access protected calls
    friend void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);
public:
    explicit QMosquitto(QObject *parent = 0);
    virtual ~QMosquitto();

    void connectTo(const QString &remote, const int port = 1883);

    void close();

    void subscribe(const QString &topic);
    void unsubscribe(const QString &topic);

    /** Start loop */
    void start();
    /** Stop loop */
    void stop(bool force = false);


signals:

    void onMessage(QString topic, QString message);

public slots:
};

#endif // MOSQUITTO_H
