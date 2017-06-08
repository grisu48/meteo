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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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

string strTime(const long milliseconds) {
	char text[100];
	time_t now = milliseconds/1000L;
	struct tm *t = localtime(&now);


	strftime(text, sizeof(text)-1, "%d.%m.%Y %H:%M:%S", t);
	return string(text);
}

long getSystemTime(void) {
	struct timeval tv;
	int64_t result;
	gettimeofday(&tv, 0);
	result = tv.tv_sec;
	result *= 1000L;
	result += (int64_t)tv.tv_usec/1000L;
	return result;
}

/** Round float to two digits*/
string round_f(const float x) {
	char buf[128];
	sprintf(buf, "%.2f", x);
	return string(buf);
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
static pthread_t startWebserver(const int port);
static void doHttpRequest(const int fd);

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
	
	
	/** Port for webserver or -1 if disabled */
	int www_port = 8033;
	
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
	map<long, WeatherNode> weatherNodes(void) const {
		map<long, WeatherNode> nodes(this->nodes);
		return nodes;
	}
	
	WeatherNode node(const long id) const {
		map<long, WeatherNode>::const_iterator it = this->nodes.find(id);
		if(it == this->nodes.end()) return WeatherNode(0L);
		else
			return it->second;
	}
	
#if 0
	vector<WeatherNode> getNodeHistory(const long id, const long minTime, const long maxTime, const long limit = 1000L, const long offset = 0L) {
		vector<WeatherNode> ret;
		
		
		
		return ret;
	}
#endif
	
	bool hasHode(const long id) const {
		map<long, WeatherNode>::const_iterator it = this->nodes.find(id);
		return (it != this->nodes.end());
	}

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
		
		// Start webserver
		if(www_port > 0) {
			tid = startWebserver(www_port);
			cout << "Webserver started on port " << www_port << " (Link: http://127.0.0.1:" << www_port << ")" << endl;
		}
		
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




/** Socket helper for http requests */
class Socket {
private:
	int sock;
	
	ssize_t write(const void* buf, size_t len) {
		if(sock <= 0) throw "Socket closed";
		ssize_t ret = ::send(sock, buf, len, 0);
		if(ret == 0) {
			this->close();
			throw "Socket closed";
		} else if (ret < 0) 
			throw strerror(errno);
		else
			return ret;
	}
	
public:
	Socket(int sock) { this->sock = sock; }
	virtual ~Socket() { this->close(); }
	void close() {
		if(sock > 0) {
			::close(sock);
			sock = 0;
		}
	}
	
	void print(const char* str) {
	    const size_t len = strlen(str);
		if(len == 0) return;
		this->write((void*)str, sizeof(char)*len);
	}
	
	void println(const char* str) {
	    this->print(str);
		this->write("\n", sizeof(char));
	}
	
	void print(const string &str) {
	    const size_t len = str.size();
		if(len == 0) return;
		this->write((void*)str.c_str(), sizeof(char)*len);
	}
	
	void println(const string &str) {
	    this->print(str);
		this->write("\n", sizeof(char));
	}
	
	Socket& operator<<(const char c) {
		this->write(&c, sizeof(char));
		return (*this);
	}
	Socket& operator<<(const char* str) {
		print(str);
		return (*this);
	}
	Socket& operator<<(const string &str) {
		this->print(str);
		return (*this);
	}
	
	Socket& operator<<(const int i) {
	    string str = ::to_string(i);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator<<(const long l) {
	    string str = ::to_string(l);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator<<(const float f) {
	    string str = ::to_string(f);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator<<(const double d) {
	    string str = ::to_string(d);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator>>(char &c) {
		if(sock <= 0) throw "Socket closed";
		ssize_t ret = ::recv(sock, &c, sizeof(char), 0);
		if(ret == 0) {
			this->close();
			throw "Socket closed";
		} else if (ret < 0) 
			throw strerror(errno);
		else
			return (*this);
	}
	
	Socket& operator>>(String &str) {
		String line = readLine();
		str = line;
		return (*this);
	}
	
	String readLine(void) {
		if(sock <= 0) throw "Socket closed";
		stringstream ss;
		while(true) {
			char c;
			ssize_t ret = ::recv(sock, &c, sizeof(char), 0);
			if(ret == 0) {
				this->close();
				throw "Socket closed";
			} else if (ret < 0) 
				throw strerror(errno);
				
			if(c == '\n')
				break;
			else
				ss << c;
		}
		String ret(ss.str());
		return ret.trim();
	}
	
	void writeHttpHeader(const int statusCode = 200) {
            this->print("HTTP/1.1 ");
            this->print(::to_string(statusCode));
            this->print(" OK\n");
            this->print("Content-Type:text/html\n");
            this->print("\n");
	}
	
	static int createListeningSocket4(const int port) {
		const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
		if(sock < 0)
			throw strerror(errno);
	
		try {
			// Setup socket
			struct sockaddr_in address;
			address.sin_family = AF_INET;
			address.sin_addr.s_addr = INADDR_ANY;
			address.sin_port = htons (port);
	
			// Set reuse address and port
			int enable = 1;
			if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
				throw strerror(errno);
			#if 0
			if (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
				throw strerror(errno);
			#endif
		
			if (::bind(sock, (struct sockaddr *) &address, sizeof(address)) != 0)
				throw strerror(errno);
		
			// Listen
			if( ::listen(sock, 10) != 0)
				throw strerror(errno);
		} catch(...) {
			::close(sock);
			throw;
		}
		return sock;
	}
};



static void doHttpRequest(const int fd) {
	// Process http request
	Socket socket(fd);
	
	// Read request
	String line;
	vector<String> header;
	while(true) {
		line = socket.readLine();
		if(line.size() == 0) break;
		else
			header.push_back(String(line));
	}
	
	// Search request for URL
	String url;
	String protocol = "";
	for(vector<String>::const_iterator it = header.begin(); it != header.end(); ++it) {
		String line = *it;
		vector<String> split = line.split(" ");
		if(split[0].equalsIgnoreCase("GET")) {
			if(split.size() < 2) 
				url = "/";
			else {
				url = split[1];
				if(split.size() > 2)
					protocol = split[2];
			}
		}
	}
	
	// Now process request
	if(!protocol.equalsIgnoreCase("HTTP/1.1")) {
		socket << "Unsupported protocol";
		socket.close();
		return;
	}
	
	// Switch requests uris
	if(url == "/" || url == "/index.html" || url == "/index.html") {
	
        socket.writeHttpHeader();
        socket << "<html><head><title>meteo Sensor node</title></head>";
        socket << "<body>";
        socket << "<h1>Meteo Server</h1>\n";
		socket << "<p><a href=\"/index.html\">[Startpage]</a> <a href=\"/nodes\">[Sensor nodes]</a> <a href=\"/current\">[Current readings]</a> </p>";
		socket << "<p>No contents yet available</p>";
		
	} else if(url == "/nodes" || url == "/current" || url == "/readings") {
		
        socket.writeHttpHeader();
        socket << "<html><head><title>meteo Sensor node</title></head>";
        socket << "<body>";
        socket << "<h1>Meteo Server</h1>\n";
		socket << "<p><a href=\"/index.html\">[Startpage]</a> <a href=\"/nodes\">[Sensor nodes]</a> <a href=\"/current\">[Current readings]</a> </p>";
		map<long, WeatherNode> nodes = instance.weatherNodes();
		if(nodes.size() == 0) {
			socket << "<p>Currently no nodes available</p>\n";
		} else {
			socket << "<p>" << (long)nodes.size() << " nodes</p>\n<table border=\"1\">";
			socket << "<tr><td><b>Node</b></td><td><b>Temperature</b></td><td><b>Pressure</b></td><td><b>Humidity</b></td><td><b>Light Visible</b></td><td><b>Light IR</b></td></tr>\n";
			for(map<long, WeatherNode>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
				socket << "<tr><td><a href=\"/nodes/" << it->second.id << "\">Node " << it->second.id << "</a></td>";
				socket << "<td>" << round_f(it->second.t) << " deg C</td>";
				socket << "<td>" << round_f(it->second.p) << " hPa</td>";
				socket << "<td>" << round_f(it->second.hum) << " % rel</td>";
				socket << "<td>" << round_f(it->second.light_vis) << "</td>";
				socket << "<td>" << round_f(it->second.light_ir) << "</td>";
				socket << "</tr>\n";
			}
			socket << "</table>\n";
		}
		
	} else if(url.startsWith("/nodes/")) {
		
		// Get id
		String sId = url.substr(7);
		long id = ::atol(sId.c_str());
		
		if(!instance.hasHode(id)) {
		    socket.writeHttpHeader(400);
		    socket << "<html><head><title>meteo Sensor node</title></head>";
		    socket << "<body>";
		    socket << "<h1>Meteo Server</h1>\n";
			socket << "<p><a href=\"/index.html\">[Startpage]</a> <a href=\"/nodes\">[Sensor nodes]</a> <a href=\"/current\">[Current readings]</a> </p>";
			socket << "<p><b>Error</b> Node not found</p>\n";
		} else {
			WeatherNode node = instance.node(id);
		
		    socket.writeHttpHeader();
		    socket << "<html><head><title>meteo Sensor node</title></head>";
		    socket << "<body>";
		    socket << "<h1>Meteo Server</h1>\n";
			socket << "<p><a href=\"/index.html\">[Startpage]</a> <a href=\"/nodes\">[Sensor nodes]</a> <a href=\"/current\">[Current readings]</a> </p>";
			socket << "<h3>Node " << id << " - Current readings</h3>\n";
			
			socket << "<table border=\"1\">\n";
			socket << "<tr><td><b>Node</b></td><td><b>Temperature</b></td><td><b>Pressure</b></td><td><b>Humidity</b></td><td><b>Light Visible</b></td><td><b>Light IR</b></td></tr>\n";
			socket << "<tr><td><a href=\"/nodes/" << node.id << "\">Node " << node.id << "</a></td>";
			socket << "<td>" << round_f(node.t) << " deg C</td>";
			socket << "<td>" << round_f(node.p) << " hPa</td>";
			socket << "<td>" << round_f(node.hum) << " % rel</td>";
			socket << "<td>" << round_f(node.light_vis) << "</td>";
			socket << "<td>" << round_f(node.light_ir) << "</td>";
			socket << "</tr>\n";
			socket << "</table>\n";
			
			socket << "<p>Timestamp: " << strTime(node.timestamp) << "</p>\n";
			
			socket << "<h3>Last 12h</h3>\n";
			
			socket << "<canvas id=\"chrtTemperature\" width=\"400\" height=\"400\"></canvas>";
			socket << "<canvas id=\"chrtPressure\" width=\"400\" height=\"400\"></canvas>";
			socket << "<canvas id=\"chrtHumidity\" width=\"400\" height=\"400\"></canvas>";
			socket << "<canvas id=\"chrtLight\" width=\"400\" height=\"400\"></canvas>";
			
			// TODO: Insert jsCharts: http://www.chartjs.org
			
		}
		
		
	} else if(url == "/plain") {
		
		socket << "HTTP/1.1 200 OK\n";
		socket << "Content-Type: text/plain\n\n";
		
        socket << "Not yet ready";
		
	} else {
		// Not found
		socket << "HTTP/1.1 404 Not Found\n";
		socket << "Content-Type: text/html\n\n";
		socket << "<html><head><title>Not found</title></head><body><h1>Not found</h1>";
		socket << "<p>Error 404 - Page not found. Maybe you want to <a href=\"index.html\">go back to the homepage</a></p>";
	}
}

struct {
	int port;
} typedef www_config_t;

static void* www_thread(void* arg) {
	www_config_t config;
	memcpy(&config, arg, sizeof(www_config_t));
	free(arg);
	
	// Create socket
	int sock = Socket::createListeningSocket4(config.port);

	// Webserver instance
	while(sock > 0) {
		const int fd = ::accept(sock, NULL, 0);
		if(fd < 0) break;		// Terminate
		
		const pid_t pid = fork();
		if(pid < 0) {
			cerr << "Http server (Port " << config.port << ") - Cannot fork: " << strerror(errno) << endl;
			::close(sock);
			exit(EXIT_FAILURE);
		} else if(pid == 0) {
			// Do http request
			::close(sock);
			
			doHttpRequest(fd);
			
			::close(fd);
			exit(EXIT_SUCCESS);
		} else {
			::close(fd);
			int status;
			// Wait for child
			waitpid(pid, &status, 0);
		}
	}
	
	::close(sock);
	return NULL;
}

static pthread_t startWebserver(const int port) {
	www_config_t *config = (www_config_t*)malloc(sizeof(www_config_t));
	if(config == NULL) throw "Cannot allocate memory";
	
	
	config->port = port;
	
	pthread_t tid;
	int ret = pthread_create(&tid, NULL, www_thread, (void*)config);
	if(ret != 0) throw "Cannot create thread";
	if(pthread_detach(tid) != 0) throw "Error detaching www thread";
	return tid;
}
