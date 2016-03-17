/* =============================================================================
 * 
 * Title:         HESS Meteo Monitor Server
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
 
#include <string>
#include <sstream>
#include <vector>

#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>


namespace hess {

class Server {
private:
	/** Server port */
	int port;
	/** Server socket */
	volatile int server_fd;
	/** Current readings */
	shm_t *shm;
	/** Get the current readings as message */
	std::string getMessage(void);
public:
	Server();
	Server(int port, shm_t *shm);
	virtual ~Server();
	
	/** Set the default port */
	void setPort(int port);
	/** Set information source (shared memory segment) */
	void setShm(shm_t*);
	/** Starts the server */
	void start();
	/** Starts the server at the given port */
	void start(int port);
	
	/** Close the server */
	void close();
};



Server::Server() {
	this->port = 0;
	this->server_fd = 0;
	this->shm = NULL;
}


Server::Server(int port, shm_t *shm) {
	this->port = port;
	this->server_fd = 0;
	this->shm = shm;
}

Server::~Server() {
	this->close();
}

void Server::setPort(int port) {
	this->port = port;
}

void Server::setShm(shm_t *shm) {
	this->shm = shm;
}

void Server::start() {
	if(this->server_fd > 0) return;
	
	
	
	
	// Socket setup
	this->server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if(this->server_fd < 0) throw (const char*)strerror(errno);
	
	// Bind to address and port
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(this->port);
	if (bind(this->server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		const char* msg = (const char*)strerror(errno);
		::close(this->server_fd);
		throw msg;
	}
	
	
	// Server loop
	int rc;
	const int server_fd = this->server_fd;
	rc = ::listen(server_fd, 10);
	if(rc < 0) throw (const char*)strerror(errno);
	while(this->server_fd > 0) {		// While not closed
		int sock = ::accept(server_fd, NULL, NULL);
		if(sock < 0) continue;
		
		// Write current readings
		std::string msg = this->getMessage();
		::write(sock, msg.c_str(), sizeof(char) * msg.length());
		
		// Done request
		::close(sock);
	}
}

std::string Server::getMessage(void) {
	std::stringstream ss;
	
	
	shm_t memory;
	memcpy(&memory, this->shm, sizeof(shm_t));
	
	ss << memory.state << " " << memory.timestamp << "\n";
	ss << memory.temperature << " " << memory.humidity << "\n";
	ss << memory.lux_broadband << " " << memory.lux_visible << " " << memory.lux_ir << "\n";
	
	std::string result = ss.str();
	return result;
}

void Server::start(int port) {
	this->port = port;
	this->start();
}

void Server::close() {
	if(this->server_fd > 0) ::close(this->server_fd);
	this->server_fd = 0;
}


} // Namespace
