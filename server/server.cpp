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
		throw "UdpReceiver already started";
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
		memset(buf, '\0', RECV_BUF_SIZE);
		// XXX: Replace this with recvfrom
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





TcpReceiver::TcpReceiver() {
	this->fd = 0;
	this->port = 0;
	this->tid = 0;
	this->recvCallback = NULL;
	
	int ret;
	ret = pthread_mutex_init(&threads_mutex, NULL);
	if(ret != 0) throw "Error creating threads mutex";
}

TcpReceiver::TcpReceiver(int port) {
	this->fd = 0;
	this->tid = 0;
	this->port = port;
	this->recvCallback = NULL;
	
	int ret;
	ret = pthread_mutex_init(&threads_mutex, NULL);
	if(ret != 0) throw "Error creating threads mutex";
	
	this->setupSocket(port);
}
	
TcpReceiver::~TcpReceiver() {
	this->close();
	pthread_mutex_destroy(&threads_mutex);
}


void TcpReceiver::addReceiverThread(TcpReceiverThread *thread) {
	const int fd = thread->getFd();
	if(pthread_mutex_lock(&this->threads_mutex) != 0) throw "Error aquiring mutex";
	
	this->threads[fd] = thread;
	
	if(pthread_mutex_unlock(&this->threads_mutex) != 0) throw "Error releasing mutex";
}

void TcpReceiver::removeReceiverThread(TcpReceiverThread *thread) {
	const int fd = thread->getFd();
	if(pthread_mutex_lock(&this->threads_mutex) != 0) throw "Error aquiring mutex";
	
	this->threads.erase(fd);
	
	if(pthread_mutex_unlock(&this->threads_mutex) != 0) throw "Error releasing mutex";
}

void TcpReceiver::received(Node &node) {
	if(this->recvCallback != NULL)
		this->recvCallback(node);
}

void TcpReceiver::setReceiveCallback( void (*recvCallback)(Node &node) ) {
	this->recvCallback = recvCallback;
}


static void *tcpReceiver_thread(void *arg) {
	// Thread is cancellable immediately
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	TcpReceiver *recv = (TcpReceiver*)arg;
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

void TcpReceiver::start(void) {
	// Check if thread is already started
	if(this->tid > 0) {
		// XXX: Error handling: Already started
		throw "TcpReceiver already started";
	}
	
	// Create thread
	int ret;
	
	pthread_t tid;
	ret = pthread_create(&tid, NULL, &tcpReceiver_thread, (void*)this);
	if(ret < 0) throw "Error creating thread";
	this->tid = tid;
}

void TcpReceiver::setupSocket(int port) {
	this->port = port;
	
	if(this->fd > 0) ::close(this->fd);
	
	// Setup receiver socket
	this->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(this->fd < 0) throw "Error setting up socket";

	struct sockaddr_in dest_addr;
	
	// Setup destination address
	memset(&dest_addr, '\0', sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = (in_port_t)htons(port);
	dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
    // set REUSE
    int enabled = 1;
    if (setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(int)) < 0) {
        ::close(this->fd);
        throw "Error setting SO_RESUSEADDR";
    }
	
	// Bind socket
	if(::bind(this->fd, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
		::close(this->fd);
		this->fd = 0;
		throw "Error binding socket";
	}
	
	// Listen
	if(::listen(this->fd, 5) < 0) {
		::close(this->fd);
		this->fd = 0;
		throw "Error listening on socket";
	}
}

void TcpReceiver::run(void) {
	if(this->fd == 0) this->setupSocket(this->port);
	
	while(this->fd > 0) {
		const int fd = ::accept(this->fd, NULL, NULL);
		if(fd < 0) break;			// Something went wrong
		else if(fd == 0) break;		// Something went wrong
		
		// Create thread for new receiver
		TcpReceiverThread *recv = new TcpReceiverThread(fd, this);
		cout << "TcpReceiver (" << this->port << ") - Connected client " << recv->getRemoteHost() << endl;
		this->addReceiverThread(recv);
	}
}

void TcpReceiver::close(void) {
	if(this->fd> 0) {
		::close(this->fd);
		this->fd = 0;
	}
	
	// Close all clients
	if(pthread_mutex_lock(&this->threads_mutex) != 0) cerr << "TcpReceiver (" << this->port << ") - Warning: Error aquiring mutex" << endl;
	
	for(map<int, TcpReceiverThread*>::iterator it = this->threads.begin(); it != this->threads.end(); it++) {
		it->second->close();		// Also deletes them
	}
	
	if(pthread_mutex_unlock(&this->threads_mutex) != 0) cerr << "TcpReceiver (" << this->port << ") - Warning: Error releasing mutex" << endl;
	
	if(this->tid > 0) {
		// Cancel receiver thread
		pthread_cancel(this->tid);
		// Wait until thread is closed
		pthread_join(this->tid, NULL);
		this->tid = 0;
	}
}


void TcpReceiver::onReceived(string msg) {
	
	Parser parser;
	if(parser.parse(msg)) {
		Node node = parser.node();
		this->received(node);
	} else {
		cerr << "Received illegal packet: " << msg << endl;
		cerr.flush();
	}
	
}

void TcpReceiver::onReceiverClosed(TcpReceiverThread *thread) {
	if(isClosed()) {
		// Just delete
		delete thread;
	} else {
		// Remove from list
		this->removeReceiverThread(thread);
		cout << "TcpReceiver (" << this->port << ") - disconnected client " << thread->getRemoteHost() << endl;
		delete thread;
	}
}

bool TcpReceiver::isClosed(void) const { return this->fd == 0; }








static void *tcp_receiver_client_thread(void *arg) {
	TcpReceiverThread *recv = (TcpReceiverThread*)arg;
    recv->run();
	return NULL;
}

TcpReceiverThread::TcpReceiverThread(int fd, TcpReceiver *receiver) {
	if(fd <= 0) throw "Illegal file descriptor";
	if(receiver == NULL) throw "No receiver parent given";
	
    this->fd = fd;
    this->receiver = receiver;
    
    // Create thread
	int ret = pthread_create(&this->tid, NULL, &tcp_receiver_client_thread, (void*)this);
	if(ret < 0) {
		throw "Error creating client thread";
	}
    pthread_detach(tid);		// Best efford - If it doesn't succeed, still continue
    
    
	// Retrieve socket information
	socklen_t len;
	struct sockaddr_storage addr;
	char ipstr[INET6_ADDRSTRLEN];

	len = sizeof addr;
	getpeername(this->fd, (struct sockaddr*)&addr, &len);

	// deal with both IPv4 and IPv6:
	if (addr.ss_family == AF_INET) {
		struct sockaddr_in *s = (struct sockaddr_in *)&addr;
		//port = ntohs(s->sin_port);
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	} else { // AF_INET6
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
		//port = ntohs(s->sin6_port);
		inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
	}

	this->remote = string(ipstr);
}

TcpReceiverThread::~TcpReceiverThread() {
    this->close();
}

void TcpReceiverThread::close(void) {
	if(this->fd == 0) return;
    if(this->fd > 0) ::close(this->fd);
    this->fd = 0;
    
    // Wait for thread to close
    if(this->tid > 0) {
	    pthread_join(this->tid, NULL);
	    this->tid = 0;
    }
    
    this->receiver->onReceiverClosed(this);
}

void TcpReceiverThread::run(void) {
    // Start receiving
    int flags = 0;
    stringstream ss;
    while(this->fd > 0) {
	    char c;
	    const ssize_t len = ::recv(fd, &c, sizeof(char), flags);
	    if(len <= 0) break;
	    if(c == '\n') {
		    string packet = ss.str();
		    ss.str("");
		    this->onReceived(packet);
	    } else
		    ss << c;
    }
}


void TcpReceiverThread::onReceived(string &input) {
	String packet(input);
	packet = packet.trim();
	
	// Check for special commands
    if(packet == "close") {
    	this->close();
    	return;
    } else {
    	// No special commands - 
    	// Try to parse the packet
		if(this->receiver == NULL) return;
		
		
		// Parse packet
		Parser parser;
		
		// 
		if(parser.parse(packet)) {
		    Node node = parser.node();
		    
		    this->receiver->received(node);
		    
		} else {
		    cerr << "TcpReceiver (" << getRemoteHost() << ", fd = " << fd << ") - Illegal input: " << packet << endl;
		}
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

std::vector<TcpBroadcastClient*> TcpBroadcastServer::getClients(void) const {
    pthread_mutex_t client_mutex = this->client_mutex;
    
	pthread_mutex_lock(&client_mutex);
	vector<TcpBroadcastClient*> ret(this->clients);
	pthread_mutex_unlock(&client_mutex);
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
	vector<TcpBroadcastClient*> clients = getClients();
	
	this->clients.clear();
	for(vector<TcpBroadcastClient*>::iterator it = clients.begin(); it != clients.end(); ++it) {
		TcpBroadcastClient *client = *it;
	    client->close();
	    delete client;
	    
	}
}

void TcpBroadcastServer::onClientDeleted(TcpBroadcastClient* client) {
	if(this->fd == 0) return;		// Closing - Do not remote from list
	
	pthread_mutex_lock(&client_mutex);
	
	// Remove from the list
	vector<TcpBroadcastClient*>::iterator it = ::find(clients.begin(), clients.end(), client);
	if(it != this->clients.end())
		this->clients.erase(it);
	
	pthread_mutex_unlock(&client_mutex);
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
		    
			pthread_mutex_lock(&client_mutex);
		    this->clients.push_back(client);
		    pthread_mutex_unlock(&client_mutex);
		    
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
    vector<TcpBroadcastClient*> clients = getClients();     // Get clients
    vector<TcpBroadcastClient*> deadClients;                // Clients that need to be erased
    
    if(clients.size() == 0) {
	    return;
    }
    
    // Broadcast to all clients
    for(vector<TcpBroadcastClient*>::iterator it = clients.begin(); it != clients.end(); ++it) {
	    TcpBroadcastClient* client = *it;
	    int ret = client->broadcast(node);
        if(ret < 0)
            deadClients.push_back(client);
    }
    
    // Delete dead clients
    for(vector<TcpBroadcastClient*>::iterator it = deadClients.begin(); it != deadClients.end(); ++it)
        delete *it;
}


void TcpBroadcastServer::onClientConnected(TcpBroadcastClient *client) {
	if(this->connectCallback != NULL)
		this->connectCallback(client);
}


void TcpBroadcastServer::onClientClosed(TcpBroadcastClient *client) {
	if(this->closedCallback != NULL)
		this->closedCallback(client);
}
