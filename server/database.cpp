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
	sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
}


SQLite3ResultSet* SQLite3Db::doQuery(string query) {
	return this->doQuery(query.c_str(), query.length());
}

SQLite3ResultSet* SQLite3Db::doQuery(const char* query, size_t len) {
	this->cleanup();
	this->rs = new SQLite3ResultSet(this, query, len);
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
	if (sqlite3_exec(db, query, NULL, NULL, &zErrMsg) != SQLITE_OK) {
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
	this->rs = new SQLite3ResultSet(this, query, len);
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
	
	rc = sqlite3_exec(db, query, NULL, NULL, &zErrMsg);
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
	rc = sqlite3_exec(db, query, rs_callback, this, &zErrMsg);
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




void SQLite3Db::push(const Node &node) {
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
               values << getMilliseconds();
 
               for(map<string, double>::iterator it = val.begin(); it != val.end(); ++it) {
                       string valName = it->first;
                       const double value = it->second;
       
                       columns << ", `" << valName << "`";
                       values << ", '" << value << '\'';
               } 
               sql << "INSERT INTO `" << name << "` (" << columns.str() << ") VALUES (" << values.str() << ");"; 
               this->exec_noResultSet(sql.str());
       }
}


std::vector<DBNode> SQLite3Db::getNodes(void) {
	SQLite3ResultSet *rs = doQuery("SELECT `id`,`name`,`location`,`description` FROM `Nodes`");


	vector<DBNode> ret;
	// Iterate over rows
	while (rs->next()) {
		const int id = rs->getInt("id");
		const std::string name = rs->getString("name");
		const std::string location = rs->getString("location");
		const std::string description = rs->getString("description");
		
		DBNode node(id, name, location, description);
		ret.push_back(node);
	}

	this->closeResultSet();
	return ret;
}

void SQLite3Db::createNodeTable(const Node &node) {
	stringstream ss;
	
	string name = escapeString(node.name());
	
	ss << "CREATE TABLE IF NOT EXISTS `Node_" << node.id() << "` ( `timestamp` INT(11)";
	
	map<string, double> val = node.values();
	for(map<string, double>::iterator it = val.begin(); it != val.end(); ++it) {
		string valName = it->first;
		ss << ", `" << valName << "` FLOAT NOT NULL";
	}
	
	ss << ", PRIMARY KEY (`timestamp`));";
	exec_noResultSet(ss.str());
	
	// TODO: Also insert into Nodes table, if not existing
	
	this->commit();
}

void SQLite3Db::createTables(void) {
	stringstream ss;
	
	ss << "CREATE TABLE IF NOT EXISTS `Nodes` (`id` INT PRIMARY KEY, `name` TEXT, `location` TEXT, `description` TEXT);";
	exec_noResultSet(ss.str());
	this->commit();
}

