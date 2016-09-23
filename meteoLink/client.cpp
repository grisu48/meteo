/* =============================================================================
 * 
 * Title:         
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <cstdlib>
#include <string>
 
#include <unistd.h>
#include <arpa/inet.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "main.hpp"

#define STR_ERROR strerror(errno)
#define RECV_BUF_SIZE 1024

using namespace std;

static int sock;
static bool running = true;


static void sig_handler(int sig_no);

int main() { //int argc, char** argv) {
	bool verbose = false;
	
	if(verbose) cout << "meteoLink Client" << endl;
    
    string endPoint = "127.0.0.1";
    int port = DEFAULT_PORT;
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGUSR1, sig_handler);
    //signal(sig_handler, SIGUSR2);
    
    // TODO: Read parameters
    
    try {
	    if(endPoint.size() <= 0) throw "Empty endpoint";
	    if(port <= 0) throw "Illegal port";
	    
	    if(verbose) cout << "Connecting to " << endPoint << ":" << port << endl;
	    
	    sock = ::socket( AF_INET, SOCK_STREAM, 0);
	    if (sock < 0) {
	    	cerr << "Error opening socket: " << STR_ERROR << endl;
	    	return EXIT_FAILURE;
	    }
	    
	    
		struct sockaddr_in m_addr;
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		in_addr_t addr = inet_addr(endPoint.c_str());
		
		memcpy( (char*)&m_addr.sin_addr,&addr, sizeof(addr));
		if(::connect(sock, (struct sockaddr*)&m_addr, sizeof(m_addr)) < 0) {
			cerr << "Error connecting to host: " << STR_ERROR << endl;
			::close(sock);
			return EXIT_FAILURE;
		}
		
		if(verbose) cout << "Receiving data ... " << endl;
		
		// Receive data
		char buf[RECV_BUF_SIZE];
		ssize_t size;
		while((size = ::recv(sock, buf, sizeof(char)*RECV_BUF_SIZE, 0)) > 0) {
			const int len = (int)size;
			for(int i=0;i<len;i++)
				cout << buf[i];
		}
		
		if(errno != 0) cerr << STR_ERROR << endl;
		if(sock > 0) ::close(sock);
		sock = 0;
	    
	    
    } catch (const char* msg) {
    	cerr << msg << endl;
    	return EXIT_FAILURE;
    }
    
    if(running)
    	return EXIT_FAILURE;
    else
	    return EXIT_SUCCESS;
}


static void sig_handler(int sig_no) {
	switch(sig_no) {
		case SIGINT:
		case SIGTERM:
		case SIGUSR1:
			signal(sig_no, sig_handler);		// Register again
			if(sock > 0 && running) {
				cerr << "Gracefully shutting down ... " << endl;
				::close(sock);
				sock = 0;
				running = false;
				return;
			} else {
				cerr << "Terminating" << endl;
				exit(EXIT_FAILURE);
			}
			return;
		case SIGUSR2:
			// TODO: Console mode
			return;
	}
}
