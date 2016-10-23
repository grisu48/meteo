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
#include <sstream>
#include <string>
#include <algorithm>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "server.hpp"
#include "string.hpp"


#define RECV_BUF_SIZE 1500			// In the size of a typical MTU



using namespace std;
using flex::String;




static int pthread_terminate(const pthread_t tid, const bool join = true) {
	if(tid == 0) return -1;
	int ret;
	
	ret = pthread_cancel(tid);
	if(ret < 0) {
		// Kill thread
		ret = pthread_kill(tid, SIGTERM);
		if(ret < 0) return ret;
	}
	if(join)
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
	if(this->fd> 0) {
		int fd = this->fd;
		this->fd = 0;
		::close(fd);
	}
	if(this->tid > 0) {
		// Wait until thread is closed
		pthread_join(this->tid, NULL);
		this->tid = 0;
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






static void* tcpClient_thread(void *arg) {
	// Thread is cancellable immediately
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	
	TcpBroadcastClient *client = (TcpBroadcastClient*)(arg);
	client->run();
	return NULL;
}

TcpBroadcastClient::TcpBroadcastClient(const int sock, TcpBroadcastServer *server) {
	if(sock <= 0) throw "Illegal socket fd";
	if(server == NULL) throw "Illegal server instance";
	
	this->sock = sock;
	this->server = server;
	
	// Retrieve socket information
	
	socklen_t len;
	struct sockaddr_storage addr;
	char ipstr[INET6_ADDRSTRLEN];
	int port;

	len = sizeof addr;
	getpeername(sock, (struct sockaddr*)&addr, &len);

	// deal with both IPv4 and IPv6:
	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
		port = ntohs(s->sin6_port);
		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}

	this->remote = string(ipstr);
	this->localPort = port;
	
	
	// Start receiver thread
	int ret;
	ret = pthread_create(&tid, NULL, &tcpClient_thread, (void*)this);
	if(ret != 0) {
		::close(sock);
		throw "Error creating client thread";
	} else {
		// Detach thread
		if(pthread_detach(tid) != 0)
			cerr << "WARNING: Error detaching client thread" << endl;
	}
	
}

TcpBroadcastClient::~TcpBroadcastClient() {
	this->close();
	
	if(server != NULL) {
		this->server->onClientDeleted(this);
	}
}

void TcpBroadcastClient::close() {
	if(this->sock == 0 && this->tid == 0) return;		// Nothing to do here

	// Close socket
	if(this->sock > 0) {
		::close(this->sock);
		this->sock = 0;
	}
	
	// Terminate thread
	if(this->tid > 0) {
		pthread_join(this->tid, NULL);
		this->tid = 0;
	}
	
	this->server->onClientClosed(this);
}

int TcpBroadcastClient::broadcast(Node &node) {
	// Send it to the client
	string xml = node.xml() + "\n";
	return (int)this->send(xml);
}

ssize_t TcpBroadcastClient::send(const char* msg, size_t len) {
	if(len <= 0) return 0;
	if(this->sock < 0) return -1;		// Error - already closed
	
	const int flags = MSG_DONTWAIT;		// Nonblocking send
	ssize_t ret = ::send(this->sock, msg, len, flags);
	if(ret < 0)
		this->close();
	return ret;
}

void TcpBroadcastClient::receivedCommand(string cmd) {
	if(receivedCallback != NULL)
		this->receivedCallback(this, cmd);
}

void TcpBroadcastClient::run(void) {
	// Wait for incoming commands
	stringstream buffer;
	char c;
	
	const int flags = 0;
	
	while(this->sock > 0) {
		// XXX: Make this more efficient by increasing buffer size
		const ssize_t len = ::recv(this->sock, &c, sizeof(char), flags);
		if(len <= 0) break;
		
		if(c == '\n') {		// Command end
			string line = buffer.str();
			buffer.str("");
			this->receivedCommand(line);
		} else
			buffer << c;
	}
	
	// Close
	this->close();
}






TcpBroadcastServer::TcpBroadcastServer(int port) {
    this->port = port;
    
    int ret;
	ret = pthread_mutex_init(&client_mutex, NULL);
	if(ret != 0)
		throw "Error creating pthread mutex";
}

TcpBroadcastServer::~TcpBroadcastServer() {
    this->close();
    pthread_mutex_destroy(&client_mutex);
}

void TcpBroadcastServer::clients_lock(void) {
	const pthread_t self_tid = pthread_self();
	static pthread_t lock_tid = 0;
    // Check if held
    int ret = pthread_mutex_trylock(&client_mutex);
    
    switch(ret) {
        case 0:				// Lock aquired successfully
        	lock_tid = self_tid;
            return;
        case EDEADLK:		// Reentrant lock
        	return;
        case EBUSY:			// Now we must wait
        	if(lock_tid == self_tid) {
        		return;
        	} else {
	        	ret = pthread_mutex_lock(&client_mutex);
	        	lock_tid = self_tid;
	        }
        default:
        	throw "Error aquiring client mutex lock";
    }
}

void TcpBroadcastServer::clients_unlock(void) {
    pthread_mutex_unlock(&client_mutex);
}

vector<TcpBroadcastClient*> TcpBroadcastServer::getClients(void) {
	this->clients_lock();
	vector<TcpBroadcastClient*> ret(this->clients);
	this->clients_unlock();
	return ret;
}

void TcpBroadcastServer::close(void) {
	if(this->fd>0) {
		int fd = this->fd;
		this->fd = 0;
		::close(fd);
	}
	
	if(this->tid > 0) {
		// Abort thread
		pthread_terminate(this->tid);
		this->tid = 0;
	}
	
	
	// Close also clients
	vector<TcpBroadcastClient*> clients(this->clients);
	this->clients.clear();

	for(vector<TcpBroadcastClient*>::iterator it = clients.begin(); it != clients.end(); ++it) {
		TcpBroadcastClient *client = *it;
	    client->close();
	    delete client;
	}
}

void TcpBroadcastServer::onClientDeleted(TcpBroadcastClient* client) {
	if(this->fd == 0) return;		// Closing
	this->clients_lock();
	
	// Remove from the list
	vector<TcpBroadcastClient*>::iterator it = ::find(clients.begin(), clients.end(), client);
	if(it != this->clients.end())
		this->clients.erase(it);
	
	this->clients_unlock();
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
        const int sock = ::accept(fd, NULL, NULL);
        if(sock <= 0) break;
        
        try {
		    TcpBroadcastClient *client = new TcpBroadcastClient(sock, this);
		    
			this->clients_lock();
		    this->clients.push_back(client);
		    this->clients_unlock();
		    
		    this->onClientConnected(client);
		} catch (const char* msg) {
			cerr << "Error attaching new client: " << msg << endl;
			break;
		}
    }    
    
    // Done
    this->close();
}

void TcpBroadcastServer::broadcast(Node &node) {
    this->clients_lock();
    this->clients_lock();
    this->clients_lock();
    
    if(clients.size() == 0) {
	   	this->clients_unlock();
	    return;
    }
    
    // Broadcast to all clients
    vector<TcpBroadcastClient*> deadClients;
    for(vector<TcpBroadcastClient*>::iterator it = clients.begin(); it != clients.end(); it++) {
	    TcpBroadcastClient* client = *it;
	    int ret = client->broadcast(node);
        if(ret < 0) {
            deadClients.push_back(client);
        } 
    }
    for(vector<TcpBroadcastClient*>::iterator it = deadClients.begin(); it != deadClients.end(); it++)
        delete (*it);
    
    
    this->clients_unlock();
}


void TcpBroadcastServer::onClientConnected(TcpBroadcastClient *client) {
	if(this->connectCallback != NULL)
		this->connectCallback(client);
}


void TcpBroadcastServer::onClientClosed(TcpBroadcastClient *client) {
	if(this->closedCallback != NULL)
		this->closedCallback(client);
}
