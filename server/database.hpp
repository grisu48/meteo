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
	
	
	std::string escape(std::string str) { return this->escape(str.c_str()); }
	std::string escape(const char* str);
public:
	MySQL(std::string remote, std::string username, std::string password, std::string database);
	virtual ~MySQL();
	
	void connect(void);
	void close(void);
	
	/** Push the given roomnode instance */
	void push(const RoomNode &node);
	
	/** Push the given node */
	void push(const Node &node);
	
	/** Get all nodes that are registered in the database */
	std::vector<DBNode> getNodes(void);
	
	std::string getDBMSVersion(void);
	
	void execute(std::string sql);
	void execute(const char* sql, size_t len);
	
	void createRoomNodeTable(const int stationId);
	void createNodeTable(const Node &node);
	
	/** Finalize mysql connection */
	static void Finalize(void);
};

#endif
