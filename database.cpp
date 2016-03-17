/* =============================================================================
 * 
 * Title:         Access to sqlite3 database
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 

#include <sstream>

#include <sqlite3.h>
#include <unistd.h>
#include <sys/time.h>


#include "database.hpp"


using namespace std;

static long getTimestamp(void) {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    
    long timestamp = (long)(milliseconds / 1000LL);
    return timestamp;
}


Sqlite3::Sqlite3(const char* filename) {
	int rc = sqlite3_open(filename, &this->db);
	if(rc != SQLITE_OK) throw "Error opening database";
}

Sqlite3::~Sqlite3() {
	if(this->db != NULL) {
		int rc;
		
		while(true) {
			rc = sqlite3_close(this->db);
			if(rc == SQLITE_OK) 
				break;
			else if(rc == SQLITE_BUSY) {
				sleep(1);
				continue;
			} else
				throw "Error closing SQLite3 database";
		}
	}
	this->db = NULL;
}
	


void Sqlite3::exec(string query) {
	if (query.length() == 0) return;
	this->exec(query.c_str(), query.length());
}

void Sqlite3::exec(const char* query, size_t len) {
	char* zErrMsg;
	if(len == 0) return;
	if (sqlite3_exec(this->db, query, NULL, NULL, &zErrMsg) != SQLITE_OK) {
		stringstream errmsg;
		errmsg << zErrMsg;
		sqlite3_free(zErrMsg);
		
		string error = errmsg.str();
		throw error;
	}
}


Database::Database(std::string filename) {
	this->_filename = filename;
}

Database::Database(const char* filename) {
	this->_filename = string(filename);
}

Database::~Database() {
	
}

void Database::write(std::string table, float temperature, float humidity, float lux_broadband, float lux_visible, float lux_ir) {
	Sqlite3 db(this->_filename.c_str());
	
	long timestamp = getTimestamp();
	
	// Build query
	stringstream query;
	query << "INSERT INTO `" << table << "` (`timestamp`, `temperature`,`humidity`,`lux_broadband`,`lux_visible`,`lux_ir`) VALUES ";
	query << "('" << timestamp << "','" << temperature << "','" << humidity <<
		"','" << lux_broadband << "','" << lux_visible << "','" << lux_ir << "');";
	string sql = query.str();
	
	try {
		db.exec(sql);
	} catch (string str) {
		this->_error = str;
		throw "Error writing to database";
	}
}

std::string Database::filename(void) { return this->_filename; }
std::string Database::getErrorMessage(void) { return this->_error; }


void Database::setupStation(std::string table) {
	stringstream query;
	
	query << "CREATE TABLE IF NOT EXISTS `" << table << "` (`timestamp` INTEGER PRIMARY KEY, `temperature` FLOAT NOT NULL, `humidity` FLOAT NOT NULL, `lux_broadband` FLOAT NOT NULL, `lux_visible` FLOAT NOT NULL, `lux_ir` FLOAT NOT NULL);";
	
	
	Sqlite3 db(this->_filename.c_str());
	
	try {
		db.exec(query.str());
	} catch (string str) {
		this->_error = str;
		throw "Error writing to database";
	}
}
