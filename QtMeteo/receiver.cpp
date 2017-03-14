#include "receiver.h"

#include <iostream>

using namespace std;

Receiver::Receiver() {


    qRegisterMetaType< WeatherData >("WeatherData");

}

Receiver::~Receiver() {
    this->close();
}

void Receiver::close(void) {
    {
        foreach(TcpReceiver *recv, this->tcpReceivers) {
            recv->close();
            delete recv;
        }
        this->tcpReceivers.clear();
        foreach(UdpReceiver *recv, this->udpReceivers) {
            recv->close();
            delete recv;
        }
        this->udpReceivers.clear();
    }
}

TcpReceiver* Receiver::addTcpReceiver(const QString &remoteHost, const int port) {
    TcpReceiver *recv = new TcpReceiver(remoteHost, port, this);
    recv->reconnect();

    if(recv == NULL) return NULL;
    connect(recv, SIGNAL(onDataArrival(WeatherData)), this, SLOT(onDataArrival(WeatherData)));
    this->tcpReceivers.append(recv);
    return recv;
}

UdpReceiver* Receiver::addUdpReceiver(const int port) {
    UdpReceiver *recv = new UdpReceiver(port);

    if(recv == NULL) return NULL;
    this->udpReceivers.append(recv);
    connect(recv, SIGNAL(onDataArrival(WeatherData)), this, SLOT(onDataArrival(WeatherData)));
    return recv;
}


void Receiver::removeReceiver(TcpReceiver *receiver) {
    this->tcpReceivers.removeAll(receiver);
}

void Receiver::onDataArrival(const WeatherData &data) {
    // Redirect arrival
    emit dataArrival(data);
}
