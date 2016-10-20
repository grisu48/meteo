#include "receiverthread.h"

#include <iostream>

using namespace std;

#define RECV_BUFFER 256

ReceiverThread::ReceiverThread(const QString &hostName, quint16 port, QObject *parent) : QThread(parent)
{
    // Try to connect
    this->socket.connectToHost(hostName, port);
    if (!socket.waitForConnected())
        throw "Connection failed";
    this->running = true;
}

ReceiverThread::~ReceiverThread() {
    this->close();
}

void ReceiverThread::close() {
    this->socket.close();
    this->running = false;
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
    QDataStream in(&socket);
    QString packet;

    while(running) {
        if(!socket.waitForReadyRead()) {
            emit error(socket.error(), socket.errorString());
            return;
        }

        // Receive data
        while(!in.atEnd()) {
            char c = '\0';
            if(in.readRawData(&c, 1) < 1) break;
            if(c == '\0') continue;
            else if (c == '\n') {
                this->packetReceived(packet);
                packet = "";
            } else {
                packet += c;
            }
        }
    }
}
