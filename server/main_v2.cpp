/* =============================================================================
 * 
 * Title:         meteo Server - Version 2, Mosquitto based
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Reception, collection and storage of meteo data
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/wait.h>

#include "string.hpp"
#include "serial.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "database.hpp"
#include "config.hpp"
#include "mosquitto.hpp"
#include "json.hpp"


using namespace std;
using flex::String;
using io::Serial;
using json = nlohmann::json;


#define DELETE(x) { if(x!=NULL) delete x; x = NULL; }

long getSystemTime(void) {
	struct timeval tv;
	int64_t result;
	gettimeofday(&tv, 0);
	result = tv.tv_sec;
	result *= 1000L;
	result += (int64_t)tv.tv_usec/1000L;
	return result;
}

/**
  * Extract remote, port, username and password from a input string.
  * The input string can be of the from [username[:password]@]remote[:port]
  
  * @param input Input string to be parsed
  * @return true if the extraction is valid
  */
static bool extractRemote(const string &input, string &remote, int &port, string &username, string &password) {
	String in = input;
	
	size_t index = in.find('@');
	if(index != string::npos) {
		String tmp(input.substr(0,index));
		in = input.substr(index+1);
		tmp = tmp.trim();
		
		index = tmp.find(':');
		if(index == string::npos) {
			username = tmp;
		} else {
			username = tmp.substr(0,index);
			password = tmp.substr(index+1);
		}
	}
	
	index = in.find(':');
	if(index == string::npos) {
		remote = in;
	} else {
		remote = in.substr(0,index);
		string sPort = in.substr(index+1);
		if(sPort.size() == 0) return false;
		else {
			port = ::atoi(sPort.c_str());
			if(port < 0 || port > 65535) return false;
		}
	}
	
	if(remote.size() == 0) return false;
	return true;
}

static void recv_callback(const std::string &topic, char* buffer, size_t len);

static void* db_thread_call(void *arg);

/** Node instance */
class WeatherNode {
public:
	/** Data timestamp */
	long timestamp;
	
	/** ID of the node */
	long id;
	/** Temperature in degree Celsius */
	float t;
	/** Pressure in hPa */
	float p;
	/** Humidity in %rel*/
	float hum;
	/** Visible light */
	float light_vis;
	/** Infrared-light */
	float light_ir;
	
	WeatherNode(long id = 0L, float t = 0.0F, float p = 0.0F, float hum = 0.0F, float light_vis = 0.0F, float light_ir = 0.0F) {
		this->timestamp = getSystemTime();
		this->id = id;
		this->t = t;
		this->p = p;
		this->hum = hum;
		this->light_vis = light_vis;
		this->light_ir = light_ir;
	}
	
	WeatherNode(const WeatherNode &node) {
		this->timestamp = node.timestamp;
		this->id = node.id;
		this->t = node.t;
		this->p = node.p;
		this->hum = node.hum;
		this->light_vis = node.light_vis;
		this->light_ir = node.light_ir;
	}
	
	/**
	  * Compute average of this node and the given node.
	  * @param alpha Weighting parameter, how relevant the given node is in respect to this node
	  * @returns reference to this Node instance
	  */
	WeatherNode& average(const WeatherNode &node, const float alpha = 0.5F) {
		if(node.timestamp > this->timestamp) this->timestamp = node.timestamp;
		this->t = alpha*this->t + (1.0F-alpha) * node.t;
		this->p = alpha*this->p + (1.0F-alpha) * node.p;
		this->hum = alpha*this->hum + (1.0F-alpha) * node.hum;
		this->light_vis = alpha*this->light_vis + (1.0F-alpha) * node.light_vis;
		this->light_ir = alpha*this->light_ir + (1.0F-alpha) * node.light_ir;
		return *this;
	}
	
	WeatherNode& set(const WeatherNode &node) {
		this->timestamp = node.timestamp;
		this->t = node.t;
		this->p = node.p;
		this->hum = node.hum;
		this->light_vis = node.light_vis;
		this->light_ir = node.light_ir;
		return *this;
	}
	
	void println(void) const {
		cout << "Node " << id << ":  " << t << " deg C, " << p << " hPa, " << hum << " % rel, " << light_vis << "/" << light_ir << " light" << endl;
	}
};

/**Program instance class */
class Instance {
private:
	/** Serial interfaces */
	vector<Serial*> serials;
	/** Mosquitto listener instances */
	vector<Mosquitto*> mosquittos;
	
	/** Database filename */
	string database = "meteod.db";
	
	/** Database instance */
	SQLite3Db *db = NULL;
	
	/** Running flag */
	volatile bool running = false;
	
	/** Nodes known to the system */
	map<long, WeatherNode> nodes;
	
	/** Write every 30 seconds */
	int db_write_interval = 30;
	
protected:
	void waitRunning() {
		if(running) return;
		else {
			// XXX: Busy waiting. No good
			while(!running)
				sleep(1);
		}
	}
	
	void db_thread_run(void) {
		if(db == NULL) {
			cerr << "Database thread reports that database has not been initialized" << endl;
			return;
		}
		
		long lastRun = 0L;		// Last write. Ignore nodes that are older
		while(running) {
			// Copy nodes
			int writes = 0;
			map<long, WeatherNode> nodes(this->nodes);
			for(map<long, WeatherNode>::const_iterator it = nodes.begin(); it != nodes.end(); it++) {
				if(it->second.timestamp < lastRun) continue;
				else {
					writeNode(it->second);
					writes++;
				}
			}
			if(writes > 0) {
				//cout << "DB thread - " << writes << " writes" << endl;
			}
			
			lastRun = getSystemTime();
			::sleep(db_write_interval);
		}
		
	}
	
public:
	bool addSerial(const char* device) {
		try {
			cout << "Serial: " << device << endl;
			
			Serial *serial = new Serial(device);
			serials.push_back(serial);
			return true;
		} catch (const char* msg) {
			cerr << "Error setting up serial device '" << device << "': " << msg << endl;
			return false;
		}
	}
	
	bool addMosquitto(const string &remote, const int port = 1883, const string username = "", const string password = "") {
		Mosquitto *mosquitto = NULL;
		cout << "Mosquitto: " << remote << ":" << port << endl;
		
		try {
			mosquitto = new Mosquitto(remote, port);
			if(username.size() > 0) mosquitto->setUsername(username);
			if(password.size() > 0) mosquitto->setPassword(password);
			this->mosquittos.push_back(mosquitto);
			mosquitto->setReceiveCallback(recv_callback);
			mosquitto->connect();
			mosquitto->subscribe("meteo/#");
			mosquitto->start();
		} catch (const char* msg) {
			DELETE(mosquitto);
			cerr << "Error adding mosquitto: " << msg << endl;
			return false;
		}
		return true;
	}
	
	bool applyConfig(const char* filename, bool mustExists = false) {
		bool valid = true;
		
		ifstream f_in(filename);
		if(!f_in.is_open()) return !mustExists;
		String line;
		long lineNumber = 0;
		while(getline(f_in, line)) {
			try {
				lineNumber++;
				line = line.trim();
				if(line.isEmpty()) continue;
				char c = line.at(0);
				if(c=='#' || c == ':') continue;
		
				size_t index = line.find('=');
				if(index == string::npos) continue;
				String name = line.substr(0,index);
				String value = line.substr(index+1);
				name = name.trim();
				value = value.trim();
		
				if(name.isEmpty()) continue;
		
				if(name == "serial") {
					addSerial(value.c_str());
				} else if(name == "database") {
					this->database = value;
				} else if(name == "mosquitto") {
					string remote;
					int port;
					string username, password;
					if(!extractRemote(value, remote, port, username, password))
						throw "Illegal mosquitto remote";
					
					
					this->addMosquitto(remote, port, username, password);
					
				} else {
					throw "Illegal instructions";
				}
			} catch (const char* msg) {
				cerr << "Config file " << filename << " - line " << lineNumber << " - " << msg << endl;
					valid = false;
			}
		}
		
		return valid;
	}
	
	/** Cleanup procedures */
	void close() {
		DELETE(db);
	}
	
	/** Start worker threads */
	void start() {
		// Setup database
		this->db = new SQLite3Db(this->database);
		
		// Start threads
		pthread_t tid;
		if(pthread_create(&tid, NULL, &db_thread_call, NULL) < 0)
			throw "Error creating db thread";
		pthread_detach(tid);
		
		
		// Mark as started
		this->running = true;
	}
	
	/** Wait until program is terminated */
	void join() {
		while(this->running) {
			sleep(1);
		}
	}
	
	void onReceived(const WeatherNode &node) {
		if(this->nodes.find(node.id) == this->nodes.end()) {
			this->nodes[node.id] = node;
			node.println();
		} else {
			this->nodes[node.id].set(node);
			this->nodes[node.id].println();
		}
	}
	
	void writeNode(const WeatherNode &node) {
		// Insert into database, if existing
		if(db != NULL) {
			stringstream ss;
			ss << "CREATE TABLE IF NOT EXISTS `Node_" << node.id << "` (`timestamp` INT PRIMARY KEY, `temperature` REAL, `pressure` REAL, `humidity` REAL, `light_vis` REAL, `light_ir` REAL);";
			db->execSql(ss.str());
			ss.str("");
			
			ss << "INSERT INTO `Node_" << node.id << "`(`timestamp`, `temperature`, `pressure`, `humidity`, `light_vis`, `light_ir`) VALUES (";
			ss << node.timestamp << ", " << node.t << ", " << node.p << ", " << node.hum << ", " << node.light_vis << ", " << node.light_ir << ");";
			db->execSql(ss.str());
			ss.str("");
		}
	}
	
	
	
	
	friend void* db_thread_call(void *arg);
};
static Instance instance;

static void* db_thread_call(void *arg) {
	(void)arg;
	// XXX pthread_detach();
	instance.db_thread_run();
	cerr << "Database thread exited" << endl;
	return NULL;
}

static float js_float(json &js, const char* key, const float defaultValue = 0.0F) {
	try {
		return js[key].get<float>();
	} catch (std::out_of_range &e) {
		return defaultValue;
	} catch (...) {
		return defaultValue;
	}
}

static void recv_callback(const std::string &topic, char* buffer, size_t len) {
	(void)len;
	string message(buffer);
	if(topic.size() == 0) return;
	try {
		json js = json::parse(message);
		
		const int id = js["node_id"].get<int>();
		float temperature = 0.0F;
		float pressure = 0.0F;
		float humidity = 0.0F;
		float light_vis = 0.0F;
		float light_ir = 0.0F;
		
		temperature = js_float(js, "temperature");
		pressure = js_float(js, "pressure");
		humidity = js_float(js, "humidity");
		light_vis = js_float(js, "light_vis");
		light_ir = js_float(js, "light_ir");
		
		// (long id = 0L, float t = 0.0F, float p = 0.0F, float hum = 0.0F, float light_vis = 0.0F, float light_ir = 0.0F) {
		WeatherNode node(id, temperature, pressure, humidity, light_vis, light_ir);
		instance.onReceived(node);
		
	} catch (std::exception &e) {
		cerr << "Parse error: " << e.what() << endl;
	} catch (...) {
		cerr << message << endl;
		cerr << "Unknown parse error" << endl;
	}
}



int main() {
	try {
		if(!instance.applyConfig("meteod.cf", true)) {
			cerr << "Error reading meteod.cf" << endl;
			return EXIT_FAILURE;
		}
		if(!instance.applyConfig("/etc/meteod.cf", false)) return EXIT_FAILURE;
		
		
		
		
		instance.start();
		instance.join();
		
	
	} catch (const char* msg) {
		cerr << "Error: " << msg << endl;
		exit(EXIT_FAILURE);
		return EXIT_FAILURE;
	}	
	
	cout << "Bye" << endl;
	return EXIT_SUCCESS;
}
