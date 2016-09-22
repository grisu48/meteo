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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


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

