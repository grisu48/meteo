#include "tcpreceiver.h"

TcpReceiver::TcpReceiver(const QString &hostName, quint16 port, QObject *parent) : QObject(parent) {
    this->remote = hostName;
    this->port = port;
    this->running = false;

    connect(&this->socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
}

TcpReceiver::~TcpReceiver() {
    this->close();
}

void TcpReceiver::close(void) {
    if(this->running == true) {
        this->running = false;
        this->socket.close();
    }
}

bool TcpReceiver::reconnect() {
    if(socket.isOpen()) return true;        // Already open
    this->socket.connectToHost(this->remote, this->port);
    if (!socket.waitForConnected())
        return false;
    return true;
}

void TcpReceiver::onReadyRead() {
    QByteArray data = socket.readAll();
    if(data.size() == 0) return;
    else {
        const int len = data.size();
        for(int i=0;i<len;i++) {
            const char c = data.at(i);
            if(c == '\n') {
                this->packetReceived(packet);
                packet = "";
            } else
                packet += c;
        }
    }
}


void TcpReceiver::packetReceived(QString &packet) {
    if(packet.isEmpty()) return;

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
            if (docElem.nodeName().compare("Node")==0) {
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
                    } else if(attrName == "id") {
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
