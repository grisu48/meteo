/* =============================================================================
 * 
 * Title:         Tcp Server for meteoLonk
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */

#include <iostream>
#include <string>
 
#include <unistd.h>
#include <arpa/inet.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

#include "tcp.hpp"

#define MAX_CONNECTIONS 10


using namespace std;


TcpServer::TcpServer(int port) {
	this->_port = port;
	this->_sock = 0;
	this->_tid = 0;
}

TcpServer::~TcpServer() {
	this->close();
}


bool TcpServer::setSocketOption(const int option, const int value) {
	int flag = value;
	int optname = option;
	if (::setsockopt(this->_sock, SOL_SOCKET, optname, &flag, sizeof(int)) < 0)
		return false;
	else
		return true;
}


static void threadStart(void* srv) {
	TcpServer *server = (TcpServer*)srv;
	server->serverMainLoop();
}

void TcpServer::serverMainLoop(void) {
	struct sockaddr_in m_addr;
	int addr_length = sizeof(m_addr);
	
	cout << "TcpServer at 0.0.0.0:" << this->_port << " [fd = " << this->_sock << "]" << endl;	
	
	// Running flag
	while(this->_sock > 0) {
		
		
		int sock = ::accept( this->_sock, (sockaddr*)&m_addr, (socklen_t*)&addr_length);
		if(sock < 0) {
			// Check if the socket is closed
			if(this->_sock <= 0) break;
			
			// XXX: Error handling
			cerr << "Error accepting socket: " << strerror(errno) << endl;
		} else {
			this->_clients.push_back(sock);		// Client accepted
		}
		
		
		
	}
}

void TcpServer::start(void) {
	if(this->_sock > 0) return;
	if(this->_tid > 0) {
		// Terminate thread
		pthread_cancel(this->_tid);
		this->_tid = 0;
	}
	
	// Setup server socket
	this->_sock = ::socket( AF_INET, SOCK_STREAM, 0);
	if(this->_sock < 0) throw "Error creating socket";
	
	
	struct sockaddr_in m_addr;
	memset(&m_addr,0,sizeof(m_addr));
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = INADDR_ANY;
	m_addr.sin_port=htons(this->_port);
	
	// Set socket options
	this->setSocketOption(SO_REUSEADDR, 1);
	this->setSocketOption(SO_REUSEPORT, 1);
	
	if (::bind( this->_sock, (sockaddr*)&m_addr, sizeof(m_addr) ) < 0) {
		::close(this->_sock);
		this->_sock = 0;
		throw "Socket binding failed";
	}
	
	
	if(::listen(this->_sock, MAX_CONNECTIONS) < 0) {
		::close(this->_sock);
		this->_sock = 0;
		throw "Listening at socket failed";
	}
	
	int ret = pthread_create(&this->_tid, 0, (void* (*)(void*))threadStart, this);
	if(ret < 0)
		throw "Error spawning server thread";
}

void TcpServer::close(void) {
	if(this->_sock > 0) 
		::close(this->_sock);
	this->_sock = 0;		// Is also the running flag
	
	// Close thread
	if(this->_tid > 0) {
		pthread_cancel(this->_tid);
		this->_tid = 0;
	}
	
	
	// Close all clients
	for(vector<int>::iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
		::close(*it);
	this->_clients.clear();
}

void TcpServer::broadcast(const char* msg, size_t size) {
	const int flags = 0;
	vector<int> validClients;
	for(vector<int>::iterator it = this->_clients.begin(); it != this->_clients.end();) {
		const int fd = *it;
		ssize_t ret = ::send(fd, msg, size, flags);
		if(ret < 0) {
			::close(fd);
			it = this->_clients.erase(it);
		} else
			++it;
	}
}

void TcpServer::broadcast(const std::string &msg) {
	this->broadcast(msg.c_str(), msg.size());
}
