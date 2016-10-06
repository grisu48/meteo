/* =============================================================================
 * 
 * Title:         Meteo Database access
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Access to the database
 * 
 * =============================================================================
 */
 
#ifndef _METEO_DATABASE_HPP_
#define _METEO_DATABASE_HPP_

#include <string>
#include <vector>
#include <map>

#include <mysql.h>

#include "node.hpp"

/** METEO database access */
class MySQL {
protected:
	std::string remote;
	std::string database;
	std::string username;
	std::string password;
	int port = 3306;
	
	
	MYSQL *conn = NULL;
public:
	MySQL(std::string remote, std::string username, std::string password, std::string database);
	virtual ~MySQL();
	
	void connect(void);
	void close(void);
	
	/** Push the given roomnode instance */
	void push(const RoomNode &node);
	
	void createRoomNodeTable(const int stationId);
	
	std::string getDBMSVersion(void);
	
	void execute(std::string sql);
	void execute(const char* sql, size_t len);
};

#endif
