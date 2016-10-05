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
}
	
UdpReceiver::~UdpReceiver() {
	this->close();
}

void UdpReceiver::received(string node, map<string, double> values) {
	cout << node;
	for(map<string, double>::iterator it = values.begin(); it != values.end(); it++) {
		string name = it->first;
		const double value = it->second;
		
		cout << " " << name << " = " << value;
	}
	cout << endl;
	
	if(this->recvCallback != NULL) {
		this->recvCallback(node, values);
	}
}

void UdpReceiver::setReceiveCallback( void (*recvCallback)(std::string, std::map<std::string, double>) ) {
	this->recvCallback = recvCallback;
}

static void *udpReceiver_thread(void *arg) {
	UdpReceiver *recv = (UdpReceiver*)arg;
	if(recv == NULL) return NULL;
	
	recv->run();
	recv->close();
	
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

void UdpReceiver::run(void) {
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
	
	
	
	// Start receiving
	char buf[RECV_BUF_SIZE+1];
	int flags = 0;
	while(this->fd > 0) {
		// XXX: Replace this with recvfrom
		memset(buf, '\0', RECV_BUF_SIZE+1);
		const ssize_t len = ::recv(this->fd, buf, RECV_BUF_SIZE * sizeof(char), flags);
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
		pthread_kill(this->tid, SIGTERM);
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
		this->received(parser.node(), parser.values());
	} else {
		cerr << "Received illegal packet: " << msg << endl;
		cerr.flush();
	}
	
}
