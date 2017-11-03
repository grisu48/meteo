#include "qmosquitto.h"

#include <string>

#include <string.h>
#include <errno.h>

#include <mosquitto.h>

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
    (void)mosq;
    QMosquitto *qmosq = (QMosquitto*)obj;

    QString topic(msg->topic);
    QString message((char*)msg->payload);

    qmosq->onReceived(topic, message);
}

QMosquitto::QMosquitto(QObject *parent) : QObject(parent)
{
    mosquitto_lib_init();
    this->mosq = mosquitto_new(NULL, true, this);
    if(this->mosq == NULL) throw "Error creating mosquitto object";

    // Register message callback
    mosquitto_message_callback_set(this->mosq, on_message);
}

QMosquitto::~QMosquitto() {
    this->close();


    if(this->mosq != NULL)
        mosquitto_destroy(this->mosq);
    this->mosq = NULL;
}

void QMosquitto::connectTo(const QString &remote, const int port) {
    this->close();
    std::string s_remote = remote.toStdString();
    int ret = mosquitto_connect(this->mosq, s_remote.c_str(), port, 30);
    if(ret == MOSQ_ERR_SUCCESS)
        return;
    else if(ret == MOSQ_ERR_ERRNO)
        throw strerror(errno);
    else if(ret == MOSQ_ERR_INVAL)
        throw "Illegal parameters";
    else
        throw "Unknown error";
}


void QMosquitto::close() {
    if(this->mosq != NULL) {
        this->stop(true);
        mosquitto_disconnect(this->mosq);
    }
}


void QMosquitto::subscribe(const QString &topic) {
    std::string s_topic = topic.toStdString();
    int rc = mosquitto_subscribe(this->mosq, NULL, s_topic.c_str(), 0);
    if(rc != MOSQ_ERR_SUCCESS)
        throw "Subscribe failed";
}

void QMosquitto::unsubscribe(const QString &topic) {
    std::string s_topic = topic.toStdString();
    int rc = mosquitto_unsubscribe(this->mosq, NULL, s_topic.c_str());
    if(rc != MOSQ_ERR_SUCCESS)
        throw "Unsubscribe failed";
}


void QMosquitto::start() {
    if(this->mosq == NULL) return;
    if(running) throw "Already running";
    int rc = mosquitto_loop_start(this->mosq);
    if(rc != MOSQ_ERR_SUCCESS) throw "Error starting loop";
    running = true;
}

void QMosquitto::stop(bool force) {
    if(this->mosq == NULL) return;
    if(!running) return;
    int rc = mosquitto_loop_stop(this->mosq, force);
    if(rc != MOSQ_ERR_SUCCESS) throw "Error starting loop";
    running = false;
}

void QMosquitto::onReceived(QString topic, QString message) {
    emit onMessage(topic, message);
}
