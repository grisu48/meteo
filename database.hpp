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
 
 
#include <string>
#include <sqlite3.h>


/** Simple Sqlite3 access */
class Sqlite3 {
private:
	sqlite3 *db;
	
public:
	Sqlite3(const char* filename);
	virtual ~Sqlite3();
	
	void exec(std::string sql);
	void exec(const char* sql, size_t len);

	/** Get the last error message */
	std::string getErrorMessage(void);
};


class Database {
private:
	std::string _filename;
	
	/** Last error message */
	std::string _error;
	
	
public:
	Database(std::string filename);
	Database(const char* filename);
	virtual ~Database();
	
	std::string filename(void);
	
	void setupStation(std::string table);
	
	void write(std::string table, float temperature, float humidity, float lux_broadband, float lux_visible, float lux_ir);
	
	/** Get the last error message */
	std::string getErrorMessage(void);
};
