/* =============================================================================
 * 
 * Title:         Meteo Node
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Node collecting data
 * 
 * =============================================================================
 */

#include <sstream>
#include <iostream>

#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "database.hpp"

using namespace std;


// #define DEBUG 1

#define DELETE(x) { if (x!=NULL) delete x; x = NULL; }
#define MARK_USED(x) (void)(x);


long getMilliseconds(void) {
	struct timeval tv;
	int64_t result;
	gettimeofday(&tv, 0);
	result = tv.tv_sec;
	result *= 1000L;
	result += (int64_t)tv.tv_usec/1000L;
	return result;
}


SQLite3Db::SQLite3Db(const char *filename) {
	this->filename = string(filename);
	this->initialize();
}

SQLite3Db::SQLite3Db(string filename) {
	this->filename = filename;
	this->initialize();
}

SQLite3Db::~SQLite3Db() {
	this->close();
	if(pthread_mutex_destroy(&mutex) <0) {
		cerr << "Error destroying pthread_mutex for sqlite3 db" << endl;
	}
}

void SQLite3Db::initialize(void) {
	int rc;
	
	this->rs = NULL;
	rc = sqlite3_open(this->filename.c_str(), &this->db);
	if(rc != SQLITE_OK) {
		stringstream errmsg;
		errmsg << "Error opening sqlite3 database " << sqlite3_errmsg(db);
		throw SQLException(errmsg.str());
	}
	
	if( pthread_mutex_init(&mutex, NULL) < 0) throw "Error creating thread mutex";
}
   
void SQLite3Db::lock() {
#if 0		// Currently not used
	if(pthread_mutex_lock(&mutex) < 0) throw "Error locking mutex";
#endif
}

void SQLite3Db::unlock() {
#if 0
	if(pthread_mutex_unlock(&mutex) < 0) throw "Error unlocking mutex";
#endif
}

void SQLite3Db::connect(void) {
	if(isConnected()) return;
	else
		this->initialize();
}

bool SQLite3Db::isClosed(void) {
	return db == NULL;
}

bool SQLite3Db::isConnected(void) {
	return db != NULL;
}
    
void SQLite3Db::close(void) {
	this->clear();
	if(db != NULL) {
		this->commit();
		sqlite3_close(db);
	}
	db = NULL;
}
    
void SQLite3Db::clear(void) {
	this->closeResultSet();
}

void SQLite3Db::closeResultSet(void) {
	if(this->rs != NULL) {
		delete this->rs;
		this->rs = NULL;
	}
}

void SQLite3Db::cleanup(void) {
	this->closeResultSet();
}

void SQLite3Db::commit(void) {
	lock();
	sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
	unlock();
}


SQLite3ResultSet* SQLite3Db::doQuery(string query) {
	return this->doQuery(query.c_str(), query.length());
}

SQLite3ResultSet* SQLite3Db::doQuery(const char* query, size_t len) {
	this->cleanup();
	lock();
	try {
		this->rs = new SQLite3ResultSet(this, query, len);
	} catch (...) { unlock(); throw; }
	unlock();
	return this->rs;
}


void SQLite3Db::execSql(std::string sql) {
	this->execSql(sql.c_str(), sql.length());
}

void SQLite3Db::execSql(const char* query, size_t len) {
	this->cleanup();
	this->rs = NULL;
	char* zErrMsg;
	if(len <= 0) return;
	
	lock();
	int rc;
	while(true) {
		rc = sqlite3_exec(db, query, NULL, NULL, &zErrMsg);
		if(rc == SQLITE_BUSY) continue;
		else break;
	}
	unlock();
	
	if (rc != SQLITE_OK) {
		stringstream errmsg;
		errmsg << "SQL error " << zErrMsg;
		sqlite3_free(zErrMsg);
		throw SQLException(errmsg.str());
	}
}

unsigned int SQLite3Db::execUpdate(std::string sql) {
	return this->execUpdate(sql.c_str(), sql.length());
}

unsigned int SQLite3Db::execUpdate(const char* query, size_t len) {
	return (unsigned int)(this->execUpdate_ul(query, len));
}

unsigned long SQLite3Db::execUpdate_ul(std::string sql) {
	return this->execUpdate_ul(sql.c_str(), sql.length());
}

unsigned long SQLite3Db::execUpdate_ul(const char* query, size_t len) {
	this->cleanup();
	lock();
	try {
		this->rs = new SQLite3ResultSet(this, query, len);
		unlock();
	} catch (...) { unlock(); throw; }
	return 0L;
}
   
void SQLite3Db::exec_noResultSet(std::string query) {
	this->exec_noResultSet(query.c_str(), query.length());
}

void SQLite3Db::exec_noResultSet(const char* query, size_t len) {
	if(db == NULL) throw SQLException("Sqlite connection closed ");
	char *zErrMsg = 0;
	int rc;	
	
	this->cleanup();
	if(len <= 0) return;
	
	lock();
	while(true) {
		rc = sqlite3_exec(db, query, NULL, NULL, &zErrMsg);
		if(rc == SQLITE_BUSY) continue;
		else break;
	}
	unlock();
	if( rc != SQLITE_OK ) {
		stringstream errmsg;
		errmsg << "Error executing sqlite command: " << zErrMsg;
		sqlite3_free(zErrMsg);
		throw SQLException(errmsg.str());
	}
}

string SQLite3Db::escapeString(const char* str, size_t len) {
	string temp(str,len);
	return this->escapeString(temp);
}

static bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

string SQLite3Db::escapeString(std::string str) {
	string result(str);
	
	replace(result, "'", "\\'");
	return result;
}


static int rs_callback(void *result_set, int argc, char **argv, char **azColName) {
	SQLite3ResultSet *rs = (SQLite3ResultSet*)(result_set);
	
	if(rs->columnNames.size() == 0) {
		rs->columns = argc;
		for(int i=0;i<argc;i++) {
			string columnName = string(azColName[i]);
			rs->columnNames.push_back(columnName);
		}
	}
	
	vector<string> rowData;
	for(int i=0;i<argc;i++)
		rowData.push_back(string(argv[i]));
	rs->rowsData.push_back(rowData);
	
	rs->rowCount++;
	return 0;
}

SQLite3ResultSet::SQLite3ResultSet(SQLite3Db* sqlite, const char *query, size_t len) {
	MARK_USED(len);
	char *zErrMsg = 0;
	sqlite3* db = sqlite->db;
	int rc;
	
	this->parent = sqlite;
	this->c_row = 0;
	this->rowCount = 0;
	
	// Execute query. Keep in mind that this call MUST be after all
	// variables have been initialized
	while(true) {
		rc = sqlite3_exec(db, query, rs_callback, this, &zErrMsg);
		if(rc == SQLITE_BUSY) continue;
		else break;		// Currently busy waiting
	}
	
	if( rc != SQLITE_OK ) {
		stringstream errmsg;
		errmsg << "Error executing sqlite command: " << zErrMsg;
		sqlite3_free(zErrMsg);
		throw SQLException(errmsg.str());
	}
}


int SQLite3ResultSet::getFieldCount(void) {
	return this->columns;
}

std::string SQLite3ResultSet::getColumnName(int index) {
	return this->columnNames[index];
}

unsigned long SQLite3ResultSet::getRowCount() {
	return this->rowCount;
}

bool SQLite3ResultSet::next() {
	if(this->c_row >= this->rowCount) return false;
	this->row = this->rowsData[this->c_row];
	this->c_row++;
	return true;
}

int SQLite3ResultSet::getColumnIndex(string name) {
	for(int i=0;i<columns;i++) 
		if(columnNames[i] == name) return i;
	
	return -1;
}

string SQLite3ResultSet::getString(int col) {
	if(this->c_row > this->rowCount) throw SQLException("End of stream");
	if(col < 0 || (unsigned int)col >= this->row.size()) throw SQLException("Column index not found");
	return this->row[col];
}

string SQLite3ResultSet::getString(string name) {
	const int index = this->getColumnIndex(name);
	if(index < 0) throw SQLException("Column name not found");
	return this->getString(index);
}

void SQLite3ResultSet::close(void) {
	parent->closeResultSet();
}

SQLite3ResultSet::~SQLite3ResultSet() {
	parent->rs = NULL;
}


