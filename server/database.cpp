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
#include <iostream>

#include <string.h>

#include "database.hpp"

using namespace std;


// #define DEBUG 1

#define DELETE(x) { if (x!=NULL) delete x; x = NULL; }

static void throwSqlError(MYSQL *conn) {	
	const char* error = mysql_error(conn);
	if(error == NULL) throw "Unknown database error";
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
	if(this->conn != NULL)
		this->close();		// Close if already established
	
#if DEBUG
		cerr << "MySQL::Connect = ";
#endif
	
	conn = mysql_init(NULL);
	if(conn == NULL) throw "Insufficient memory";
	mysql_real_connect(conn, this->remote.c_str(), this->username.c_str(), this->password.c_str(), this->database.c_str(), this->port, NULL, 0);
	if(!conn) {
#if DEBUG
			cerr << "Err" << endl;
#endif
		throwSqlError(this->conn);
	}
#if DEBUG
		cerr << "OK" << endl;
#endif
}

void MySQL::commit(void) {
	if(this->conn != NULL) {
#if DEBUG
		cerr << "MySQL::Commit = ";
#endif
		mysql_commit(this->conn);
#if DEBUG
		cerr << "OK" << endl;
#endif
	}
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
			values << ", '" << value << '\'';
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

vector<DBNode> MySQL::getNodes(void) {
	execute("SELECT `id`,`name`,`location`,`description` FROM `Nodes`");

	MYSQL_RES *res = mysql_use_result(conn);
	if(res == NULL) throw "Error getting result structure";
	MYSQL_ROW row;

	vector<DBNode> ret;
	// Iterate over rows
	while ((row = mysql_fetch_row(res)) != NULL) {
		string s_id = string(row[0]);
		string name = string(row[1]);
		string location = string(row[2]);
		string description = string(row[3]);
		
		const int id = atoi(s_id.c_str());
		ret.push_back(DBNode(id,name,location,description));
	}

	mysql_free_result(res);

	return ret;
}

void MySQL::createNodeTable(const Node &node) {
	stringstream ss;
	
	string name = escape(node.name());
	
	ss << "CREATE TABLE IF NOT EXISTS `" << this->database << "`.`Node_" << node.id() << "` ( `timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP";
	
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

void MySQL::checkError(void) const throw (SQLException) {
    if(this->conn == NULL) return;
    
    if (mysql_errno(this->conn) == 0) return;
    throw SQLException(mysql_error(this->conn));
}

void MySQL::execute(std::string sql) {
	this->execute(sql.c_str(), sql.size());
}

void MySQL::execute(const char* sql, size_t len) {
	int ret;
	
	if(this->conn == NULL) connect();
	
#if DEBUG
	cerr << "MySQL::execute(\"" << sql << "\") = ";
#endif
	
	ret = mysql_real_query(this->conn, sql, len);
	if(ret != 0) {
		const char* error = mysql_error(this->conn);
#if DEBUG
		//const int errno = mysql_errno(this->conn);
		
		cerr << " ERR (" << error << ")" << endl;
#endif
		
		
		if(error == NULL) throw "Unknown error";
		else throw error;
	} 
#if DEBUG
	cerr << ret << endl;
#endif
}

void MySQL::Finalize(void) {
	mysql_thread_end();
	mysql_library_end();
}



MySQLResultSet::MySQLResultSet(MySQL* mysql, string query) {
    this->mysql = mysql;
    this->query = query;
    
    /* Actual do query */
    int result = mysql_real_query(mysql->conn, query.c_str(), query.length());
    mysql->checkError();
    
    switch(result) {
        case 0: break;     // All good
#if 0
        case CR_COMMANDS_OUT_OF_SYNC:
            throw SQLException("Command out of sync");
        case CR_SERVER_GONE_ERROR:
            throw SQLException("Server is gone");
        case CR_SERVER_LOST:
            throw SQLException("Connection lost");
        //case CR_UNKNOWN_ERROR:
#endif
        default:
            throw SQLException("Unknown error");
    }
    
    
    this->mysql_res = mysql_store_result(this->mysql->conn);
    mysql->checkError();
    rows = (unsigned long) mysql_num_rows (mysql_res);
    mysql->checkError();
    this->affectedRows = (unsigned long)mysql_affected_rows(mysql->conn);
    mysql->checkError();
}

MySQLResultSet::~MySQLResultSet() {
	close();
}

bool MySQLResultSet::isClosed(void) {
	return mysql_res == NULL;
}

void MySQLResultSet::close(void) {
	if(isClosed()) return;
    mysql_free_result(mysql_res);
    mysql_res = NULL;
    
    for(vector<MySQLField*>::iterator it = fields.begin(); it != fields.end(); ++it )
        delete *it;
    fields.clear();
    rowData.clear();
}

unsigned long MySQLResultSet::getAffectedRows() {
    return this->affectedRows;
}

unsigned long MySQLResultSet::getRowCount() {
    return rows;
}

bool MySQLResultSet::next() {
    if(this->mysql_res == NULL) throw new SQLException();
    
    MYSQL_ROW  row;
    if ((row = mysql_fetch_row (mysql_res)) == NULL) 
        return false;
    mysql->checkError();
    
    // Fetch all data
    rowData.clear();
    for(vector<MySQLField*>::iterator it = fields.begin(); it != fields.end(); ++it )
        delete *it;
    fields.clear();
    
    
    mysql_field_seek (mysql_res, 0);
    mysql->checkError();
    this->columns = mysql_num_fields(mysql_res);
    for(unsigned int i=0;i<columns;i++) {
        rowData.push_back(row[i]);
        
        MySQLField *field = new MySQLField();
        my_field = mysql_fetch_field (mysql_res);
        mysql->checkError();
        
        this->columnNames.push_back(std::string(my_field->name));
        
        field->name = string(my_field->name);
        field->type = my_field->type;
        field->length = my_field->length;
        field->max_length = my_field->max_length;
        field->flags = my_field->flags;
        field->decimals = my_field->decimals;

        fields.push_back(field);
    }
    
    return true;
}


int MySQLResultSet::getFieldCount(void) {
	return (int)this->columns;
}

std::string MySQLResultSet::getColumnName(int index) {
	return this->columnNames[index];
}

std::string MySQLResultSet::getString(string name) {
    int col = getFieldIndex(name);
    if(col < 0) throw SQLException("Column not found");
    return this->getString(col);
}

std::string MySQLResultSet::getString(int col) {
    if(this->mysql_res == NULL) throw new SQLException();
    if(col < 0 || (unsigned int)col >= rowData.size()) throw SQLException("Outside column index");
    return rowData[col];
}

int MySQLResultSet::getFieldIndex(string name) {
    int i = 0;
    for(vector<MySQLField*>::iterator it = fields.begin(); it != fields.end(); ++it ) { 
        MySQLField* field = *it;       
        
        if(field->name == name) return i;
        
        // Next
        i++;
    }
    return -1;
}


