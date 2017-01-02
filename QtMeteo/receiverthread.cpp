#include "receiverthread.h"

#include <iostream>
#include <string>

using namespace std;

// Maximum receive buffer
#define RECV_BUFFER 1024

ReceiverThread::ReceiverThread(const QString &hostName, quint16 port, QObject *parent) : QThread(parent)
{
    // Try to connect
    this->remote = hostName;
    this->port = port;
    this->running = false;
}

ReceiverThread::~ReceiverThread() {
    this->close();
}

bool ReceiverThread::reconnect() {
    if(socket.isOpen()) return true;        // Already open
    this->socket.connectToHost(this->remote, this->port);
    if (!socket.waitForConnected())
        return false;
    return true;
}

void ReceiverThread::close() {
    this->running = false;
    this->wait();
    this->socket.close();
}


void ReceiverThread::queryNodes() {
    this->socket.write("list\n", 5);
}

void ReceiverThread::packetReceived(QString &packet) {
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
            long id = 0L;
            QString name = "";
            QMap<QString, double> values;
            if (docElem.nodeName().compare("Node")==0) {
                QDomNamedNodeMap attributes = docElem.attributes();

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
                        double value = attr.value().toDouble(&ok);
                        if(ok) {
                            values[attrName] = value;
                        }
                    }
                }

                // Check if all values are filled
                if (id <= 0L) throw "Illegal id";
                if(values.size() == 0) return;      // Empty packet


                emit newData(id, values);
            } else {
                throw "Illegal node";
            }
        }
    } catch (const char* msg) {
        QString message(msg);
        emit parseError(message, packet);
    }
}

void ReceiverThread::run() {
    this->running = reconnect();
    if(!this->running)
        throw "Connection failed";


    QString packet;

    while(running) {
        if(!socket.waitForReadyRead(1000L)) {
            if(!running) return;

            if(!reconnect()) {
                emit error(socket.error(), socket.errorString());
                return;
            } else {
                continue;
            }
        }
        if(socket.bytesAvailable() <= 0) continue;

        QByteArray data = socket.readAll();
        if(data.size() == 0) continue;
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
}
