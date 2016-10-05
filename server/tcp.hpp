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
 
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
 
class TcpServer {
private:
private:
	int _port;
	int _sock;
	std::vector<int> _clients;
	
	/** Thread ID for the worker  */
	pthread_t _tid;
	
	
	/**
	 * Set socket options
	 * @param option Option to be set, see man setsockopt for details
	 * @param value to be set
	 * @return true if successfull, false on error
	 */
	bool setSocketOption(const int option, const int value);
public:
	/** Setup the Tcp server without listening
	  */
	TcpServer(int port);
	virtual ~TcpServer();
	
	/** Start listening for incoming connections.
	  * This call will fork a thread, that is responsible for hanlding incoming
	  * connections.
	  */
	void start(void);
	
	/**
	  * Enter server main loop. This call is intended to be called by the
	  * server thread
	  */
	void serverMainLoop(void);

	void close(void);

	/** Broadcast a single message */
	void broadcast(const char* msg, size_t size);

	void broadcast(const std::string &msg);
};


