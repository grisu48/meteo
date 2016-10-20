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


class TcpBroadcastServer {
private:
    /** Server FD */
    volatile int fd;
    
    /** Port */
    int port;
    
    /** Attached client sockets */
    std::vector<int> clients;
    
	/** Server thread ID */
	volatile pthread_t tid = 0;
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
    
    void broadcast(std::string msg) { this->broadcast(msg.c_str(), msg.size()); }
    void broadcast(const char* msg, size_t len);
};

#endif
