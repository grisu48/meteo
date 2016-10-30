/* =============================================================================
 * 
 * Title:         meteo Server
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Reception, collection and storage of meteo data
 *                This file needs support for POSIX threads (-pthread)
 * 
 * =============================================================================
 */

#ifndef _METEO_SERVER_HPP_
#define _METEO_SERVER_HPP_

#include <string>
#include <vector>
#include <map>

#include <pthread.h>

#include "udp.hpp"
#include "parser.hpp"
#include "node.hpp"

class TcpBroadcastClient;
class TcpBroadcastServer;
class UdpReceiver;
class TcpReceiver;


/** Udp packet receiver instance */
class UdpReceiver {
private:
	/** Socket for receiving */
	volatile int fd;
	
	int port;
	
	/** Receiver thread ID */
	volatile pthread_t tid;
	
	/** When a message has been received */
	void onReceived(std::string msg);
	
	/** Received callback */
	void (*recvCallback)(Node &node);
	
	void setupSocket(int port);
public:
	/** Creates new empty receiver */
	UdpReceiver();
	/** Creates new udp receiver on the given port */
	UdpReceiver(int port);
	virtual ~UdpReceiver();
	
	/** Virtual method when new data has been received */
	virtual void received(Node &node);
	
	/** Sets the receive callback */
	void setReceiveCallback( void (*recvCallback)(Node &node) );
	
	/** Starts the receiver thread */
	void start(void);
	
	/** Receiver loop.
	  * This call should be executed by the Thread. You should call start() from external, unless you know what you are doing
	  */
	void run(void);
	
	/** Closes the receiver */
	void close(void);
};




/** Internal receiver class */
class TcpReceiverThread {
private:
    /** Socket file descriptor */
    int fd;
    
    /** Thread id */
    pthread_t tid;
    
    /** Parent receiver*/
    TcpReceiver *receiver;
    
    std::string remote;
    
    void onReceived(std::string &packet);
    
public:
    TcpReceiverThread(int fd, TcpReceiver *receiver);
    virtual ~TcpReceiverThread();
    
    int getFd(void) const { return this->fd; }
    
    /** Close this thread */
    void close(void);
    
    /** Receive loop */
    void run(void);
    
    std::string getRemoteHost(void) const { return this->remote; }
};


/** Udp packet receiver instance */
class TcpReceiver {
private:
	/** Socket for receiving */
	volatile int fd;
	
	int port;
	
	/** Receiver thread ID */
	volatile pthread_t tid;
	
	/** All active receiver threads. Map the file descriptor (fd) to TcpReceiverThread* instance */
	std::map<int, TcpReceiverThread*> threads;
	
	/** Adds a receiver thread */
	void addReceiverThread(TcpReceiverThread *thread);
	/** Remove receiver thread */
	void removeReceiverThread(TcpReceiverThread *thread);
	
	/** Mutex for access to threads vector */
	pthread_mutex_t threads_mutex;
	
	/** When a message has been received */
	void onReceived(std::string msg);
	
	/** Received callback */
	void (*recvCallback)(Node &node);
	
	void setupSocket(int port);
	
	/** This method is called, when a thread is closed */
	void onReceiverClosed(TcpReceiverThread *thread);
public:
	/** Creates new empty receiver */
	TcpReceiver();
	/** Creates new udp receiver on the given port */
	TcpReceiver(int port);
	virtual ~TcpReceiver();
	
	/** Virtual method when new data has been received */
	virtual void received(Node &node);
	
	/** Sets the receive callback */
	void setReceiveCallback( void (*recvCallback)(Node &node) );
	
	/** Starts the receiver thread */
	void start(void);
	
	/** Receiver loop.
	  * This call should be executed by the Thread. You should call start() from external, unless you know what you are doing
	  */
	void run(void);
	
	/** True if the instance has been closed or is about to be closed */
	bool isClosed(void) const;
	
	/** Closes the receiver */
	void close(void);
	
	friend class TcpReceiverThread;
};




/** A client instance for the TcpBroadcastServer */
class TcpBroadcastClient {
private:
	/** Parent server instance */
	TcpBroadcastServer *server = NULL;
	
	/** Callback for received messages */
	void (*receivedCallback)(TcpBroadcastClient *client, std::string) = NULL;
protected:
	/** Client socket */
	volatile int sock;
	
	/** Receiver thread ID */
	pthread_t tid = 0;
	
	/** Remote host */
	std::string remote;
	
	/** Local port */
	int localPort;
	
	/** Handle a command, the client has sent */
	virtual void receivedCommand(std::string cmd);
public:
	/** Initialize new broadcast client*/
	TcpBroadcastClient(const int fd, TcpBroadcastServer *server);
	
	virtual ~TcpBroadcastClient();
	
	/** This call is intended to be executed by the thread */
	void run(void);
	
	
    /** Broadcast an update of a given node */
    int broadcast(Node &noe);
    
    void close(void);
    
    ssize_t send(const char* msg, size_t len);
    ssize_t send(std::string msg) { return this->send(msg.c_str(), msg.size()); }
    
    void setReceivedCallback(void (*callback)(TcpBroadcastClient *client, std::string msg)) { this->receivedCallback = callback; }
    
    /** Get the remote host of the connection */
    std::string getRemoteHost(void) const { return remote; }
    int getLocalPort(void) const { return localPort; }
};

/** Server for broadcasting data */
class TcpBroadcastServer {
private:
    /** Server FD */
    volatile int fd;
    
    /** Port */
    int port;
    
    /** Attached client sockets */
    std::vector<TcpBroadcastClient*> clients;
    
	/** Mutex for access to client vector */
	pthread_mutex_t client_mutex;
	
	/** Server thread ID */
	volatile pthread_t tid = 0;
	
	/** Callback when a client closes in order to dispose it from the list */
	void onClientDeleted(TcpBroadcastClient* client);
	
	/** Connected callback */
	void (*connectCallback)(TcpBroadcastClient *client) = NULL;
	void (*closedCallback)(TcpBroadcastClient *client) = NULL;
	
protected:
	virtual void onClientConnected(TcpBroadcastClient *client);
	virtual void onClientClosed(TcpBroadcastClient *client);
public:
    TcpBroadcastServer(int port);
    virtual ~TcpBroadcastServer();
    
    /** Closes server and all clients */
    void close(void);
    
    /** Start the server */
    void start(void);
    
	/** Receiver loop.
	  * This call should be executed by the Thread. You should call start() from external, unless you know what you are doing
	  */
    void run(void);
    
    void setConnectedCallback(void (*callback)(TcpBroadcastClient *client)) { this->connectCallback = callback; }
    void setClosedCallback(void (*callback)(TcpBroadcastClient *client)) { this->closedCallback = callback; }
    
    /** Broadcast an update of a given node */
    void broadcast(Node &node);
    
    friend class TcpBroadcastClient;
};

#endif
