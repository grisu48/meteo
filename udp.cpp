/* =============================================================================
 * 
 * Title:         Simple UDP socket
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */

#include <string>
#include <vector>

class UdpServer {
private:
	int fd;
	struct sockaddr_in dest_addr;
	int port;
	std::string remote;
	
public:
	UdpServer(std::string remote, int port);
	virtual ~UdpServer();
	
	
	
	
	void close(void);
	ssize_t send(const char* msg, size_t size);
	ssize_t send(std::string msg);	
};


UdpServer::UdpServer(std::string remote, int port) {
	this->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(this->fd < 0) throw "Error setting up socket";
	int broadcastEnable=1;
	int ret=setsockopt(this->fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
	if(ret != 0) throw "Error enabling broadcast";	

	this->port = port;
	this->remote = remote;
	
	// Setup destination address
	memset(&dest_addr, '\0', sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = (in_port_t)htons(port);
	//dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	dest_addr.sin_addr.s_addr = inet_addr(remote.c_str());
}

UdpServer::~UdpServer() {
	this->close();
}


void UdpServer::close() {
	if(this->fd > 0) ::close(this->fd);
	this->fd = 0;
}


ssize_t UdpServer::send(const char* msg, size_t size) {
	ssize_t ret;
	int flags = 0;
	
	const socklen_t dest_len = sizeof(dest_addr);
	
	
	ret = sendto(this->fd, (void*)msg, sizeof(char)*size, flags, (const sockaddr*)&dest_addr, dest_len);
	return ret;
}

ssize_t UdpServer::send(std::string msg) {
	return this->send(msg.c_str(), msg.length());
}
