/** Collector instance */

#include "collector.hpp"
#include "time.hpp"

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <errno.h>

using namespace std;



static bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

string escapeString(std::string str) {
	string result(str);
	
	replace(result, "'", "\\'");
	return result;
}




static void* collector_thread_run(void *arg) {
	Collector *collector = (Collector*)arg;
	collector->run();
	return NULL;
}

Collector::Collector() {
	if(pthread_mutex_init(&mutex, NULL) < 0) throw "Error creating thread mutex";
}

Collector::~Collector() {
	this->close();
	pthread_mutex_destroy(&mutex);
}


void Collector::push(long id, std::string name, float t, float hum, float p, float l_ir, float l_vis) {
	if(stations.find(id) == stations.end()) {
		stations[id] = Station(id);
		stations[id].name = name;
	}
	if(stations[id].name == "" && name != "")
		stations[id].name = name;
	
	stations[id].push(t, hum, p, l_ir, l_vis, f_alpha);
}

void Collector::purge_stations() {
	map<long, Station>::iterator it = stations.begin();
	
	while(it != stations.end()) {
		if (!it->second.alive())
			it = stations.erase(it);
		else
			++it;
	}
}

void Collector::start() {
	if(running) throw "Collector already started";
	running = true;
	
	if(pthread_create(&this->tid, NULL, collector_thread_run, this) < 0)
		throw "Error creating collector thread";
	if(pthread_detach(this->tid) < 0) throw "Error detaching thread";
}


void Collector::mutex_lock() const {
	if(pthread_mutex_lock((pthread_mutex_t*)&this->mutex) < 0) throw "Error locking mutex";
}

void Collector::mutex_unlock() const {
	if(pthread_mutex_unlock((pthread_mutex_t*)&this->mutex) < 0) throw "Error locking mutex";
}

void Collector::run() {
	while(running) {
		sleep(delay);
		
		mutex_lock();
		this->purge_stations();
		
		int64_t timestamp = (lazy::getSystemTime()/1000L);
		
		// Push station data and mark as non-alive
		for(map<long,Station>::iterator it = stations.begin(); it != stations.end(); ++it) {
			it->second.alive(false);

			// Push to database			
			try {
				stringstream sql;
				sql << "CREATE TABLE IF NOT EXISTS `station_" << it->second.id << "` (`timestamp` INT PRIMARY KEY, `temperature` REAL, `humidity` REAL, `pressure` REAL, `light_ir` REAL, `light_vis` REAL);";
				sql_exec(sql.str());
				sql.str("");
				sql << "INSERT OR IGNORE INTO `stations` (`id`, `name`, `desc`) VALUES (" << it->second.id << ",";
				sql << "'" << escapeString(it->second.name) << "', '');";
				sql_exec(sql.str());
				sql.str("");
				
				sql << "INSERT OR REPLACE INTO `station_" << it->second.id << "` (`timestamp`,`temperature`,`humidity`,`pressure`,`light_ir`,`light_vis`) VALUES (";
				sql << timestamp << "," << it->second.t << "," << it->second.hum << "," << it->second.p << "," << it->second.l_ir << "," << it->second.l_vis << ");";
				sql_exec(sql.str());
			} catch (const char *msg) {
				cerr << msg << endl;
				continue;
			}
			
		}
		mutex_unlock();
	}
}


void Collector::push(const Lightning &lightning) {	
	mutex_lock();
	
	// Push to database			
	try {
		stringstream sql;
		
		sql << "CREATE TABLE IF NOT EXISTS `lightnings_" << lightning.station << "` (`timestamp` INT PRIMARY KEY, `distance` REAL);";
		sql_exec(sql.str());
		sql.str("");
	
		sql << "INSERT OR REPLACE INTO `lightnings_" << lightning.station << "` (`timestamp`,`distance`) VALUES (";
		sql << lightning.timestamp << ", " << lightning.distance << ");";
		sql_exec(sql.str());
		
	} catch (...) {
		mutex_unlock();
		throw;
	}
	mutex_unlock();
}

vector<Station> Collector::activeStations(void) const {
	vector<Station> stations;
	
	mutex_lock();
	for(map<long,Station>::const_iterator it = this->stations.begin(); it != this->stations.end(); ++it) 
		stations.push_back(it->second);
	mutex_unlock();
	return stations;
}

Station Collector::station(const long id) const {
	Station ret;
	mutex_lock();
	std::map<long, Station>::const_iterator it = stations.find(id);
	if(it != stations.end()) ret = it->second;
	mutex_unlock();
	return ret;
}

void Collector::close() {
	if(running)
		pthread_cancel(this->tid);
	
	running = false;
	mutex_lock();
	if(this->db != NULL) {
		sqlite3_close(this->db);
		this->db = NULL;
	}
	mutex_unlock();
}

void Collector::open_db(const char* filename) {
	if(this->db != NULL) throw "Database already opened";
	
	int rc = sqlite3_open(filename, &this->db);
	if(rc != SQLITE_OK) throw strerror(errno);
	
	sql_exec("CREATE TABLE IF NOT EXISTS `stations` (`id` INT PRIMARY KEY, `name` TEXT, `desc` TEXT);");
}

void Collector::sql_exec(const std::string sql) {
	if(this->db == NULL) throw "No database opened";
	
	char* zErrMsg;
	int rc;
	while(true) {
		rc = sqlite3_exec(db, sql.c_str(), NULL, NULL, &zErrMsg);
		if(rc == SQLITE_BUSY) continue;
		else break;
	}
	if(rc != SQLITE_OK) {
		cerr << "SQLITE3 error: " << zErrMsg << endl;
		sqlite3_free(zErrMsg);
		throw "SQL error";
	}
}


vector<DataPoint> Collector::query(const long station, const long minTimestamp, const long maxTimestamp, const long limit, const long offset) {
	if(this->db == NULL) throw "No database opened";
	
	vector<DataPoint> ret;
	mutex_lock();
	
	stringstream sql;
	sql << "SELECT `timestamp`,`temperature`,`humidity`,`pressure`,`light_ir`,`light_vis` FROM `station_" << station << "`";
	if(minTimestamp >= 0L && maxTimestamp >= 0L) {
		sql << " WHERE `timestamp` >= " << minTimestamp << " AND `timestamp` <= " << maxTimestamp;
	} else if(minTimestamp >= 0L) {
		sql << " WHERE `timestamp` >= " << minTimestamp;
	} else if(maxTimestamp >= 0L) {
	sql << " WHERE `timestamp` <= " << maxTimestamp;
	}
	sql << " ORDER BY `timestamp` DESC LIMIT " << limit << " OFFSET " << offset << ";";
	
	
	int rc;
    sqlite3_stmt *res;
    
	string query = sql.str();
	while(true) {
		rc = sqlite3_prepare_v2(this->db, query.c_str(), -1, &res, NULL);    
		if(rc == SQLITE_BUSY) continue;		// Retry if busy
		else break;
	}
	if(rc != SQLITE_OK) {
		mutex_unlock();
		
		if(errno == ENOENT) {
			return ret;
		} else {
			cerr << "SQLITE3 error: " << strerror(errno) << endl;
			throw "SQL error";
		}
	}
	
	// Iterate over rows
	while( (rc = sqlite3_step(res)) == SQLITE_ROW) {
		// double sqlite3_column_double(sqlite3_stmt*, int iCol);
		// sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol);
		
		DataPoint dp;
		
		dp.station = station;
		dp.timestamp = (long)sqlite3_column_int64(res, 0);
		dp.t = (float)sqlite3_column_double(res, 1);
		dp.hum = (float)sqlite3_column_double(res, 2);
		dp.p = (float)sqlite3_column_double(res, 3);
		dp.l_ir = (float)sqlite3_column_double(res, 4);
		dp.l_vis = (float)sqlite3_column_double(res, 5);
		
		ret.push_back(dp);
	}
	
	if(rc != SQLITE_DONE) {
		cerr << "SQLITE3 error: " << strerror(errno) << endl;
		
		mutex_unlock();
		throw "SQL error";
	}
	
	if(sqlite3_finalize(res) != SQLITE_OK) {
		cerr << "SQLITE3 finalize failed" << endl;
		mutex_unlock();
		throw "SQL error";
	}
	
	mutex_unlock();
	return ret;
}
