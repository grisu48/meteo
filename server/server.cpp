/* =============================================================================
 * 
 * Title:         Server instance
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <string>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "server.hpp"
#include "string.hpp"


#define RECV_BUF_SIZE 1500			// In the size of a typical MTU



using namespace std;
using flex::String;




static int pthread_terminate(pthread_t tid) {
	if(tid == 0) return -1;
	int ret;
	
	ret = pthread_cancel(tid);
	if(ret < 0) {
		// Kill thread
		ret = pthread_kill(tid, SIGTERM);
		if(ret < 0) return ret;
	}
	pthread_join(tid, NULL);
	return ret;
	
}


UdpReceiver::UdpReceiver() {
	this->fd = 0;
	this->port = 0;
	this->tid = 0;
	this->recvCallback = NULL;
}

UdpReceiver::UdpReceiver(int port) {
	this->fd = 0;
	this->tid = 0;
	this->port = port;
	this->recvCallback = NULL;
	this->setupSocket(port);
}
	
UdpReceiver::~UdpReceiver() {
	this->close();
}

void UdpReceiver::received(Node &node) {
	if(this->recvCallback != NULL)
		this->recvCallback(node);
}

void UdpReceiver::setReceiveCallback( void (*recvCallback)(Node &node) ) {
	this->recvCallback = recvCallback;
}


static void *udpReceiver_thread(void *arg) {
	// Thread is cancellable immediately
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	UdpReceiver *recv = (UdpReceiver*)arg;
	if(recv == NULL) return NULL;
	
	try {
		recv->run();
		recv->close();
	} catch (const char* msg) {
		cout.flush();
		cerr << msg << ": " << strerror(errno) << endl;
		cerr.flush();
	}
	
	return NULL;
}

void UdpReceiver::start(void) {
	// Check if thread is already started
	if(this->tid > 0) {
		// XXX: Error handling: Already started
		throw "TcpReceiver already started";
	}
	
	// Create thread
	int ret;
	
	pthread_t tid;
	ret = pthread_create(&tid, NULL, &udpReceiver_thread, (void*)this);
	if(ret < 0) throw "Error creating thread";
	this->tid = tid;
}

void UdpReceiver::setupSocket(int port) {
	this->port = port;
	
	if(this->fd > 0) ::close(this->fd);
	
	// Setup receiver socket
	this->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(this->fd < 0) throw "Error setting up socket";

	struct sockaddr_in dest_addr;
	
	// Setup destination address
	memset(&dest_addr, '\0', sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = (in_port_t)htons(port);
	dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//dest_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	
	
	// Bind socket
	if(::bind(this->fd, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
		::close(this->fd);
		this->fd = 0;
		throw "Error binding socket";
	}
}

void UdpReceiver::run(void) {
	if(this->fd == 0) this->setupSocket(this->port);
	
	// Start receiving
	char buf[RECV_BUF_SIZE+1];
	int flags = 0;
	while(this->fd > 0) {
		// XXX: Replace this with recvfrom
		memset(buf, '\0', RECV_BUF_SIZE);
		const ssize_t len = ::recv(this->fd, buf, RECV_BUF_SIZE * sizeof(char), flags);
		buf[RECV_BUF_SIZE] = '\0';
		if(len < 0) break;
		
		// Received message
		string msg(buf);
		this->onReceived(msg);
	}
	if(this->fd > 0) {
	
	}
}

void UdpReceiver::close(void) {
	if(this->tid > 0) {
		// Abort thread
		pthread_terminate(this->tid);
		this->tid = 0;
	}
	if(this->fd> 0) {
		int fd = this->fd;
		this->fd = 0;
		::close(fd);
	}
}


void UdpReceiver::onReceived(string msg) {
	
	Parser parser;
	if(parser.parse(msg)) {
		Node node = parser.node();
		this->received(node);
	} else {
		cerr << "Received illegal packet: " << msg << endl;
		cerr.flush();
	}
	
}




TcpBroadcastServer::TcpBroadcastServer(int port) {
    this->port = port;
}

TcpBroadcastServer::~TcpBroadcastServer() {
    this->close();
}

void TcpBroadcastServer::close(void) {
	if(this->tid > 0) {
		// Abort thread
		pthread_terminate(this->tid);
		this->tid = 0;
	}
	if(this->fd> 0) {
		int fd = this->fd;
		this->fd = 0;
		::close(fd);
	}
	// Close also clients
	for(vector<int>::iterator it = this->clients.begin(); it != this->clients.end(); ++it) {
	    ::close(*it);
	}
	this->clients.clear();
}


static void *tcpServer_thread(void *arg) {
    TcpBroadcastServer *server = (TcpBroadcastServer*)arg;
    server->run();
    return NULL;
}

void TcpBroadcastServer::start(void) {
	// Check if thread is already started
	if(this->tid > 0) {
		// XXX: Error handling: Already started
		throw "UdpReceiver already started";
	}
	
	struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    
    // Open socket
    this->fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(this->fd < 0) throw "Error opening socket";
    
    // set REUSE
    int enabled = 1;
    if (setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(int)) < 0) {
        ::close(this->fd);
        throw "Error setting SO_RESUSEADDR";
    }
    
    // Bind
     if (::bind(this->fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        ::close(this->fd);
        throw "Error binding socket";
     }
     
     // Listen
     if(::listen(this->fd,5) <0) {
        ::close(this->fd);
        throw "Error listening to socket";
     }
	
	// Create thread
	int ret;
	
	pthread_t tid;
	ret = pthread_create(&tid, NULL, &tcpServer_thread, (void*)this);
	if(ret < 0) throw "Error creating thread";
	this->tid = tid;
}

void TcpBroadcastServer::run(void) {
    
    const int fd = this->fd;
    while(this->fd > 0) {
        int client = ::accept(fd, NULL, NULL);
        // XXX: Possible race condition!
        this->clients.push_back(client);
    }    
    
    // Done
    this->close();
}

void TcpBroadcastServer::broadcast(const char* msg, size_t len) {
    if(len == 0) return;
    
    const int flags = MSG_DONTWAIT;     // NONBLOCKING
    
    // XXX: Possible race condition
    if(clients.size() == 0) return;
    // Broadcast to all clients
    for(vector<int>::iterator it = clients.begin(); it != clients.end(); ) {
        ssize_t ret = ::send(*it, msg, sizeof(char) * len, flags);
        if(ret < 0) {
            // Socket closed
            it = clients.erase(it);
        } else {
            ++it;
        }
    }
}
