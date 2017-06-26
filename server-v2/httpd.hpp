/* =============================================================================
 * 
 * Title:         meteo Server - Webserver part
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * 
 * =============================================================================
 */


#include <string>

#include "database.hpp"


/** Webserver instance for meteo */
class Webserver {
private:
	int port;
	std::string db_filename;
	
	/** Webserver listener port */
	int sock;
	
	/** Running flag */
	volatile bool running = true;
	
	void doHttpRequest(const int fd);
public:
	Webserver(const int port, const std::string &db_filename);
	virtual ~Webserver();
	
	
	void loop(void);
	
	void close() {
		const int sock = this->sock;
		this->sock = 0;
		if(sock > 0)
			::close(sock);
	}
};

