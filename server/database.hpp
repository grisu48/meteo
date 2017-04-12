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




class MySQLResultSet;
class MySQL;

/** General SQL exception */
class SQLException : public std::exception {
protected:
    /** Error message. */
    std::string _msg;
public:
    explicit SQLException(const char* msg) { this->_msg = std::string(msg); }
    explicit SQLException(std::string msg) { this->_msg = msg; }
    explicit SQLException() { this->_msg = ""; }
    virtual ~SQLException() throw () {}
    
    std::string getMessage() { return _msg; }
    virtual const char* what() const throw () {
    	return _msg.c_str();
    }
};





/** METEO database access */
class MySQL {
protected:
	std::string remote;
	std::string database;
	std::string username;
	std::string password;
	int port = 3306;
	
	MYSQL *conn = NULL;
	
    
    MySQLResultSet* resultSet;
	
	std::string escape(std::string str) { return this->escape(str.c_str()); }
	std::string escape(const char* str);
public:
	MySQL(std::string remote, std::string username, std::string password, std::string database);
	virtual ~MySQL();
	
	void connect(void);
	void close(void);
	
	void commit(void);
	
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
	
	
    MySQLResultSet* doQuery(std::string query);
    MySQLResultSet* doQuery(const char* query, size_t len);
    
    void checkError(void) const throw (SQLException);
    
    friend class MySQLResultSet;
};


class MySQLField {
public:
    std::string name;
    int type;
    unsigned int length;
    unsigned int max_length;
    unsigned int flags;
    unsigned int decimals;
};

class MySQLResultSet {
private:
    MySQL* mysql;
    MYSQL_RES  *mysql_res;
    MYSQL_FIELD *my_field;
    
    std::string query;
    std::vector<std::string> rowData;
    std::vector<MySQLField*> fields;
    
    unsigned long rows;
    unsigned long affectedRows;
    int getFieldIndex(std::string name);
    
    unsigned int columns;
    std::vector<std::string> columnNames;
    
protected:
    bool isClosed(void);
    
public:
    MySQLResultSet(MySQL* mysql, std::string query);
    virtual ~MySQLResultSet();
    
    unsigned long getRowCount();
    
    int getFieldCount(void);
    std::string getColumnName(int index);
    bool next();
    
    std::string getString(int col);
    std::string getString(std::string name);
    
    unsigned long getAffectedRows();
    
    void close(void);
};

#endif
