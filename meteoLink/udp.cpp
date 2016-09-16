/* =============================================================================
 * 
 * Title:         Udp Broadcast Template for meteo
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Class for UdpBroadcasting
 * 
 * =============================================================================
 */

#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

class DatagramSocket {
private:
	int fd;
	struct sockaddr_in dest_addr;
	int port;
	std::string remote;
	
public:
	DatagramSocket(std::string remote, int port);
	virtual ~DatagramSocket();
	
	std::string getRemoteHost(void) const { return this->remote; }
	int getPort(void) { return this->port; }
	
	/** Enable broadcast for this socket*/
	void enableBroadcast(void);
	
	/** Close this broadcast instance */
	void close(void);
	/** Send the given message */
	ssize_t send(const char* msg, size_t size);
	/** Send the given message */
	ssize_t send(std::string msg);
};


DatagramSocket::DatagramSocket(std::string remote, int port) {
	this->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(this->fd < 0) throw "Error setting up socket";

	this->port = port;
	this->remote = remote;
	
	// Setup destination address
	memset(&dest_addr, '\0', sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = (in_port_t)htons(port);
	//dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	dest_addr.sin_addr.s_addr = inet_addr(remote.c_str());
}

DatagramSocket::~DatagramSocket() {
	this->close();
}

void DatagramSocket::enableBroadcast() {
	int broadcastEnable=1;
	int ret=setsockopt(this->fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
	if(ret != 0) throw (const char*)strerror(errno);
}

void DatagramSocket::close() {
	if(this->fd > 0) ::close(this->fd);
	this->fd = 0;
}


ssize_t DatagramSocket::send(const char* msg, size_t size) {
	ssize_t ret;
	int flags = 0;
	
	const socklen_t dest_len = sizeof(dest_addr);
	
	
	ret = ::sendto(this->fd, (void*)msg, sizeof(char)*size, flags, (const sockaddr*)&dest_addr, dest_len);
	if(ret < 0) throw (const char*)strerror(errno);
	return ret;
}

ssize_t DatagramSocket::send(std::string msg) {
	return this->send(msg.c_str(), msg.length());
}
