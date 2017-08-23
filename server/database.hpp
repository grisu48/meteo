/* =============================================================================
 * 
 * Title:         Meteo Database access
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
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

#include <sqlite3.h>

#include <pthread.h>

class SQLite3ResultSet;
class SQLite3Db;

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




/** METEO Sqlite3 database (default now) */
class SQLite3Db {
protected:
	sqlite3 *db;
	std::string filename;
	
	SQLite3ResultSet *rs;
	
	void initialize(void);
	
	void closeResultSet(void);
	
	// Mutex
	pthread_mutex_t mutex;
	
	void lock();
	void unlock();
public:
	SQLite3Db(const char* filename);
	SQLite3Db(std::string filename);
	~SQLite3Db();
    
    SQLite3ResultSet* doQuery(std::string query);
    SQLite3ResultSet* doQuery(const char* query, size_t len);
    
    void execSql(std::string sql);
    void execSql(const char* query, size_t len);
    unsigned int execUpdate(std::string sql);
    unsigned int execUpdate(const char* query, size_t len);
    unsigned long execUpdate_ul(std::string sql);
    unsigned long execUpdate_ul(const char* query, size_t len);
    
    void exec_noResultSet(std::string query);
    void exec_noResultSet(const char* query, size_t len);
    
    std::string getVersion();
    SQLite3ResultSet* getResultSet();
    
    void connect(void);
    void close(void);
    void clear(void);
    void commit(void);
    void cleanup(void);
    bool isClosed(void);
    bool isConnected(void);
	
	
    std::string escapeString(std::string str);
    std::string escapeString(const char* str, size_t len);
	
    friend class SQLite3ResultSet;
};


class SQLite3ResultSet {
protected:
	SQLite3Db* parent;
	unsigned long c_row;
	std::vector<std::string> row;
	
	
	int getColumnIndex(std::string name);
public:
	/* UGLY hack: They are public so that the callback method may access them */
	int columns;
	unsigned long rowCount;
	std::vector<std::string> columnNames;
	std::vector<std::vector<std::string> > rowsData;

	SQLite3ResultSet(SQLite3Db* sqlite, const char *query, size_t len);
	virtual ~SQLite3ResultSet();
	
	int getFieldCount(void);
    std::string getColumnName(int index);
	
	unsigned long getRowCount();
	bool next();

	void close(void);

	std::string getString(int col);
	std::string getString(std::string name);
	std::string getString(const char* name) { 
		std::string s_name(name);
		return this->getString(s_name);
	}
	
	int getInt(int col) {
		std::string s = getString(col);
		return ::atoi(s.c_str());
	}
	
	int getInt(const char* name) {
		std::string s = getString(name);
		return ::atoi(s.c_str());
	}
	int getInt(const std::string &name) {
		std::string s = getString(name);
		return ::atoi(s.c_str());
	}
	
	float getFloat(int col) {
		std::string s = getString(col);
		return (float)::atof(s.c_str());
	}
	
	float getFloat(const char* name) {
		std::string s = getString(name);
		return (float)::atof(s.c_str());
	}
	float getFloat(const std::string &name) {
		std::string s = getString(name);
		return (float)::atof(s.c_str());
	}
	
	long getLong(int col) {
		std::string s = getString(col);
		return ::atol(s.c_str());
	}
	
	long getLong(const char* name) {
		std::string s = getString(name);
		return ::atol(s.c_str());
	}
	long getLong(const std::string &name) {
		std::string s = getString(name);
		return ::atol(s.c_str());
	}
	
	std::string operator[](const char* name) { return this->getString(name); }
	std::string operator[](const std::string &name) { return this->getString(name); }
};


#endif
