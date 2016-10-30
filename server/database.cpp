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

#include <string.h>

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
	if(this->conn != NULL) {
		mysql_close(this->conn);
		this->conn = NULL;
	}
}


void MySQL::push(const RoomNode &node) {
	stringstream sql;
	
	sql << "INSERT INTO `RoomNode_" << node.id() << "` (`timestamp`, `light`, `humidity`, `temperature`) ";
	sql << "VALUES (CURRENT_TIMESTAMP, '" << node.light() << "', '" << node.humidity() << "', '" << node.temperature() << "');";
	
	
	this->execute(sql.str());
}

std::string MySQL::escape(const char* str) {
	// unsigned long mysql_real_escape_string(MYSQL *mysql, char *to, const char *from, unsigned long length);
	
	if(this->conn == NULL) return string(str);
	
	size_t len = strlen(str);
	char* res = new char[len*2+1];
	memset(res, '\0', len*2+1);
	size_t ret_len = mysql_real_escape_string(this->conn, res, str, len);
	res[ret_len] = '\0';
	res[len*2] = '\0';		// Make sure it is escaped
	string ret = string(str);
	delete[] res;
	return ret;
}

void MySQL::push(const Node &node) {
	stringstream sql;
	
	const int id = node.id();
	if(id <= 0) return;
	
	string name = "Node_" + ::to_string(id);

	map<string, double> val = node.values();
		
	// Create table statement
	this->createNodeTable(node);
	
	// Insert values statement
	{
		stringstream columns;
		stringstream values;

		columns << "`timestamp`";
		values << "CURRENT_TIMESTAMP";

		for(map<string, double>::iterator it = val.begin(); it != val.end(); ++it) {
			string valName = it->first;
			const double value = it->second;
	
			columns << ", `" << valName << "`";
			values << ", " << value;
		}

		sql << "INSERT INTO `" << this->database << "`.`" << name << "` (" << columns.str() << ") VALUES (" << values.str() << ");";


		this->execute(sql.str());
	}
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

void MySQL::createNodeTable(const Node &node) {
	stringstream ss;
	
	string name = escape(node.name());
	
	ss << "CREATE TABLE IF NOT EXISTS `" << this->database << "`.`Node_" << name << "` ( `timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP";
	
	map<string, double> val = node.values();
	for(map<string, double>::iterator it = val.begin(); it != val.end(); ++it) {
		string valName = it->first;
		ss << ", `" << valName << "` FLOAT NOT NULL";
	}
	
	ss << ", PRIMARY KEY (`timestamp`)) ENGINE = InnoDB CHARSET=utf8 COLLATE utf8_general_ci COMMENT = 'Station " << name << " table';";
	execute(ss.str());	
}

void MySQL::createRoomNodeTable(const int stationId) {
	stringstream ss;
	ss << "CREATE TABLE IF NOT EXISTS `" << this->database << "`.`RoomNode_" << stationId << "` ( `timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP , `light` FLOAT NOT NULL , `humidity` FLOAT NOT NULL , `temperature` FLOAT NOT NULL , PRIMARY KEY (`timestamp`)) ENGINE = InnoDB CHARSET=utf8 COLLATE utf8_general_ci COMMENT = 'Station " << stationId << " table';";
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

void MySQL::Finalize(void) {
	// mysql_thread_end();
	mysql_library_end();
}

