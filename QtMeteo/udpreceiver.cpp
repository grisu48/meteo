#include "udpreceiver.h"

#define RECV_BUFFER 1024 * 16

UdpReceiver::UdpReceiver(quint16 port, QObject *parent) : QObject(parent)
{
    this->socket.bind(QHostAddress::Any, port);
    connect(&this->socket, SIGNAL(readyRead()), this, SLOT(onDatagramArrival()));
}


UdpReceiver::~UdpReceiver() {
    this->close();
}


void UdpReceiver::close(void) {
    this->socket.close();
}

void UdpReceiver::onDatagramArrival() {
    char buffer[RECV_BUFFER+1];
    while (socket.hasPendingDatagrams()) {
        QHostAddress host;
        quint16 port;
        socket.readDatagram(buffer, RECV_BUFFER, &host, &port);
        buffer[RECV_BUFFER] = '\0';     // Ensure it is escaped
        QString packet(buffer);
        this->processDatagram(packet, host, port);
    }
}


void UdpReceiver::processDatagram(QString packet, const QHostAddress &host, const int port) {
    packet = packet.trimmed();
    if(packet.isEmpty()) return;
    if(host.isNull()) return;
    if(port == 0) return;

    // Try to parse the packet
    try {
        if(packet.isEmpty()) {
            throw "Empty input";
        } else {
            QDomElement docElem;
            QDomDocument xmldoc;

            // Parse
            xmldoc.setContent(packet);
            docElem=xmldoc.documentElement();

            // Parse node
            if (docElem.nodeName().compare("meteo")==0) {
                QDomNamedNodeMap attributes = docElem.attributes();

                long id = 0L;
                QString name = "";
                float temperature = 0.0F;
                float humidity = 0.0F;
                float pressure = 0.0F;
                float light = 0.0F;
                long timestamp = 0L;    // XXX: Todo set timestamp

                // Read attributes
                const int nAttrs = attributes.size();
                for(int i=0;i<nAttrs;i++) {
                    QDomAttr attr = attributes.item(i).toAttr();
                    QString attrName = attr.name();

                    if(attrName == "name") {
                        name = attr.value();
                    } else if(attrName == "station") {
                        bool ok;
                        long newId = attr.value().toDouble(&ok);
                        if(ok)
                            id = newId;
                    } else {
                        bool ok;
                        const float value = attr.value().toFloat(&ok);

                        if(ok) {
                            if (attrName == "temperature")
                                temperature = value;
                            else if (attrName == "humidity")
                                humidity = value;
                            else if (attrName == "pressure")
                                pressure = value;
                            else if (attrName == "light")
                                light = value;
                        }

                    }
                }

                // Check if all values are filled
                if (id <= 0L) throw "Illegal id";

                // New data arrived
                WeatherData data(id, temperature, humidity, pressure, light, timestamp);
                emit onDataArrival(data);
            } else {
                throw "Illegal node";
            }
        }
    } catch (const char* msg) {
        QString message(msg);
        emit parseError(message, packet);
    }
}
