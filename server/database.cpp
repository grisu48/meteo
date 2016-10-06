/* =============================================================================
 * 
 * Title:         Meteo Node
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Node collecting data
 * 
 * =============================================================================
 */

#include <sstream>

#include "database.hpp"

using namespace std;

#define DELETE(x) { if (x!=NULL) delete x; x = NULL; }

static void throwSqlError(MYSQL *conn) {	
	const char* error = mysql_error(conn);
	if(error == NULL) throw "Unknown error";
	else throw error;
}

MySQL::MySQL(std::string remote, std::string username, std::string password, std::string database) {
	this->remote = remote;
	this->database = database;
	this->username = username;
	this->password = password;
	
	
}

MySQL::~MySQL() {
	this->close();
}


void MySQL::connect(void) {
	this->close();		// Close if already established
	
	conn = mysql_init(NULL);
	if(conn == NULL) throw "Insufficient memory";
	if(!mysql_real_connect(conn, this->remote.c_str(), this->username.c_str(), this->password.c_str(), this->database.c_str(), this->port, NULL, 0)) 
		throwSqlError(this->conn);
}

void MySQL::close(void) {
	mysql_close(this->conn);
}


void MySQL::push(const RoomNode &node) {
	stringstream sql;
	
	sql << "INSERT INTO `RoomNode_" << node.stationId() << "` (`timestamp`, `light`, `humidity`, `temperature`) ";
	sql << "VALUES (CURRENT_TIMESTAMP, '" << node.light() << "', '" << node.humidity() << "', '" << node.temperature() << "');";
	
	
	this->execute(sql.str());
}

string MySQL::getDBMSVersion(void) {
	execute("SELECT VERSION();");

	string version = "";
	MYSQL_RES *res = mysql_use_result(conn);
	if(res == NULL) throw "Error getting result structure";
	MYSQL_ROW row;

	// Iterate over rows
	while ((row = mysql_fetch_row(res)) != NULL) {
		version = string(row[0]);
		break;
	}

	mysql_free_result(res);

	return version;
}


void MySQL::createRoomNodeTable(const int stationId) {
	stringstream ss;
	ss << "CREATE TABLE IF NOT EXISTS `meteo`.`RoomNode_" << stationId << "` ( `timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP , `light` FLOAT NOT NULL , `humidity` FLOAT NOT NULL , `temperature` FLOAT NOT NULL , PRIMARY KEY (`timestamp`)) ENGINE = InnoDB CHARSET=utf8 COLLATE utf8_general_ci COMMENT = 'Station " << stationId << " table';";
	execute(ss.str());
}

void MySQL::execute(std::string sql) {
	this->execute(sql.c_str(), sql.size());
}

void MySQL::execute(const char* sql, size_t len) {
	int ret;
	
	ret = mysql_real_query(this->conn, sql, len);
	if(ret != 0) {
		//int errno = mysql_errno(this->conn);
		const char* error = mysql_error(this->conn);
		if(error == NULL) throw "Unknown error";
		else throw error;
	}
}

