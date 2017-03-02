/* =============================================================================
 * 
 * Title:         meteo Sensor Webserver
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Webserver instance that allows readout of the current sensors
 *                via http requests
 * =============================================================================
 */
 
 
#include <iostream>
#include <thread>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

#include <unistd.h> 
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>
#include <netinet/tcp.h>

#include "string.hpp"



using namespace std;
using flex::String;


static int createListeningSocket4(const int port);
/** Fork and process the given request */
static void doRequest(const int fd);

volatile bool running = true;
static int sock = 0;		// Server socket

/** Socket helper */
class Socket {
private:
	int sock;
	
	ssize_t write(const void* buf, size_t len) {
		if(sock <= 0) throw "Socket closed";
		ssize_t ret = ::send(sock, buf, len, 0);
		if(ret == 0) {
			this->close();
			throw "Socket closed";
		} else if (ret < 0) 
			throw strerror(errno);
		else
			return ret;
	}
	
public:
	Socket(int sock) { this->sock = sock; }
	virtual ~Socket() { this->close(); }
	void close() {
		if(sock > 0) {
			::close(sock);
			sock = 0;
		}
	}
	
	Socket& operator<<(const char c) {
		this->write(&c, sizeof(char));
		return (*this);
	}
	Socket& operator<<(const char* str) {
		const size_t len = strlen(str);
		if(len == 0) return (*this);
		this->write((void*)str, sizeof(char)*len);
		return (*this);
	}
	Socket& operator<<(const string &str) {
		const size_t len = str.size();
		if(len == 0) return (*this);
		this->write((void*)str.c_str(), sizeof(char)*len);
		return (*this);
	}
	
	Socket& operator>>(char &c) {
		if(sock <= 0) throw "Socket closed";
		ssize_t ret = ::recv(sock, &c, sizeof(char), 0);
		if(ret == 0) {
			this->close();
			throw "Socket closed";
		} else if (ret < 0) 
			throw strerror(errno);
		else
			return (*this);
	}
	
	Socket& operator>>(String &str) {
		String line = readLine();
		str = line;
		return (*this);
	}
	
	String readLine(void) {
		if(sock <= 0) throw "Socket closed";
		stringstream ss;
		while(true) {
			char c;
			ssize_t ret = ::recv(sock, &c, sizeof(char), 0);
			if(ret == 0) {
				this->close();
				throw "Socket closed";
			} else if (ret < 0) 
				throw strerror(errno);
				
			if(c == '\n')
				break;
			else
				ss << c;
		}
		String ret(ss.str());
		return ret.trim();
	}
};


int main() { //int argc, char** argv) {
    int port = 8428;		// Default port
    
    try {
	    sock = createListeningSocket4(port);
    	
    	cout << "Listening on port " << port << " ... " << endl;
    	
    	// Accept clients
    	while(running) {
    		const int fd = ::accept(sock, NULL, 0);
    		doRequest(fd);
    	}
    	
    	// Close
    	if(sock > 0) ::close(sock);
    	sock = 0;
    } catch (const char* msg) {
    	if(sock > 0) ::close(sock);
    	sock = 0;
    	cerr << "Error: " << msg << endl;
    	return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


static int createListeningSocket4(const int port) {
	const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
		throw strerror(errno);
	
	try {
		// Setup socket
		struct sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons (port);
	
		// Set reuse address and port
		int enable = 1;
		if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			throw strerror(errno);
		if (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
			throw strerror(errno);
		
		if (::bind(sock, (struct sockaddr *) &address, sizeof(address)) != 0)
			throw strerror(errno);
		
		// Listen
		if( ::listen(sock, 10) != 0)
			throw strerror(errno);
	} catch(...) {
		::close(sock);
		throw;
	}
	return sock;
}

static void doRequest(const int fd) {
	const pid_t pid = fork();
	if(pid < 0) throw strerror(errno);
	if(pid == 0) {
		::close(fd);		// We don't need the socket
		return;
	} else {
		// Process http request
		Socket socket(fd);
		
		// Read request
		String line;
		vector<String> header;
		while(true) {
			line = socket.readLine();
			if(line.size() == 0) break;
			else
				header.push_back(String(line));
		}
		
		// Search request for URL
		String url;
		String protocol = "";
		for(vector<String>::const_iterator it = header.begin(); it != header.end(); ++it) {
			String line = *it;
			vector<String> split = line.split(" ");
			if(split[0].equalsIgnoreCase("GET")) {
				if(split.size() < 2) 
					url = "/";
				else {
					url = split[1];
					if(split.size() > 2)
						protocol = split[2];
				}
			}
		}
		
		// Now process request
		if(!protocol.equalsIgnoreCase("HTTP/1.1")) {
			// XXX: Unsupported protocol
			socket.close();
			return;
		}
		
		// Switch now requests
		if(url == "/") {
			// Default page
			
			
		} else if(url == "/bmp180") {
			
		} else if(url == "/htu21df" || url == "/htu-21df") {
			
		} else if(url == "/mcp9808") {
			
		} else if(url == "/tsl2561") {
			
		} else {
			// Not found
			socket << "HTTP/1.1 404 Not Found\n";
			socket << "Content-Type: text/html\n\n";
			socket << "404 - Sensor or page not found";
		}
		
		socket.close();
	}
}


