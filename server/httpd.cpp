/* =============================================================================
 * 
 * Title:         meteo Server - Webserver part
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * 
 * =============================================================================
 */


#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "httpd.hpp"
#include "database.hpp"
#include "string.hpp"


using namespace std;
using flex::String;


/** Socket helper */
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
};




Webserver::Webserver(const int port, const std::string &db_filename) {
	this->port = port;
	this->db_filename = db_filename;
	
	// Setup socket
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
	
	this->sock = sock;
}

Webserver::~Webserver() {
	this->close();
}



class WebserverThreadParams {
public:
	int port;
	string db_filename;
};

static void alive_thread(void* arg) {
	WebserverThreadParams *params = (WebserverThreadParams*)arg;
	Webserver server(params->port, params->db_filename);
	delete params;
	
	server.loop();
	server.close();
}

pthread_t Webserver::startThreaded(const int port, const std::string &db_filename) {
	WebserverThreadParams *params = new WebserverThreadParams();
	params->port = port;
	params->db_filename = db_filename;
	
	int ret;
	pthread_t tid;
	ret = pthread_create(&tid, NULL, (void* (*)(void*))alive_thread, params);	
	if(ret < 0) {
		delete params;
		throw "Error create thread";
	}
	if(pthread_detach(tid) < 0) {
		delete params;
		throw "Error detaching thread";
	}
	return tid;
}

static int fd;
static void sig_handler(int sig_no) {
	switch(sig_no) {
	case SIGALRM:
		cerr << "Terminating http request" << endl;
		::close(fd);
		exit(EXIT_FAILURE);
		return;
	}
}
	

void Webserver::loop(void) {
	while(running && this->sock > 0) {
		fd = ::accept(sock, NULL, 0);
		if(fd < 0) break;		// Terminate
		
		const pid_t pid = fork();
		if(pid < 0) {
			throw "Cannot fork";
			::close(sock);
			exit(EXIT_FAILURE);
		} else if(pid == 0) {
			::close(sock);
			// Do http request
			alarm(2);		// Request max take at most 2 seconds
			signal(SIGALRM, sig_handler);
			this->doHttpRequest(fd);
			::close(fd);
			exit(EXIT_SUCCESS);
		} else {
			// Parent
			
			::close(fd);
#if 0			// Don't wait, immediately ready again
			int status;
			// Wait for child
			waitpid(pid, &status, 0);
#endif
		}
	}
	
	this->close();
}

class Node {
public:
	long id;
	string name;
	
	
	Node() {
		this->id = 0;
		this->name = "";
	}
	Node(const Node &node) {
		this->id = node.id;
		this->name = node.name;
	}
	
	void print(ostream &out = cout) {
		out << this->id << " [" << this->name << "]" << endl;
	}
};

class DataPoint {
public:
	long node = 0;
	long timestamp = 0L;
	
	float t = 0.0F;
	float p = 0.0F;
	float hum = 0.0F;
	float l_vis = 0.0F;
	float l_ir = 0.0F;
	
	DataPoint() {}
	DataPoint(const DataPoint &d) {
		this->node = d.node;
		this->timestamp = d.timestamp;

		this->t = d.t;
		this->p = d.p;
		this->hum = d.hum;
		this->l_vis = d.l_vis;
		this->l_ir = d.l_ir;
	}
};

vector<Node> getNodes(SQLite3Db &db) {
	SQLite3ResultSet *rs = db.doQuery("SELECT * FROM `Nodes`;");
	vector<Node> ret;
	while(rs->next()) {
		Node node;
		node.id = rs->getLong("id");
		node.name = rs->getString("name");
		ret.push_back(node);
	}
	rs->close();
	return ret;
}

vector<DataPoint> getPoints(SQLite3Db &db, const long node, const long minTimestamp = -1L, const long maxTimestamp = -1L, const long limit = 100, const long offset = 0L) {
	stringstream query;
	
	query << "SELECT `timestamp`,`t`,`hum`,`p`,`l_vis`,`l_ir` FROM `Node_" << node << "`";
	if(minTimestamp >= 0L && maxTimestamp >= 0L) {
		query << " WHERE `timestamp` >= " << minTimestamp << " AND `timestamp` <= " << maxTimestamp;
	} else if(minTimestamp >= 0L) {
		query << " WHERE `timestamp` >= " << minTimestamp;
	} else if(maxTimestamp >= 0L) {
		query << " WHERE `timestamp` <= " << maxTimestamp;
	}
	query << " ORDER BY `timestamp` DESC";
	if (limit > 0L || offset > 0L) 
		query << " LIMIT " << limit << " OFFSET " << offset;
	query << ";";
	
	string str = query.str();
	
	vector<DataPoint> ret;
	SQLite3ResultSet *rs = db.doQuery(str);
	while(rs->next()) {
		DataPoint dp;
		dp.node = node;
		dp.timestamp = rs->getLong(0);
		dp.t = rs->getFloat(1);
		dp.hum = rs->getFloat(2);
		dp.p = rs->getFloat(3);
		dp.l_vis = rs->getFloat(4);
		dp.l_ir = rs->getFloat(5);
		ret.push_back(dp);
	}
	rs->close();
	return ret;
}

vector<DataPoint> getPointsToday(SQLite3Db &db, const long node, const long limit = 1000, const long offset = 0L) {
	lazy::DateTime today;
	today.setMilliseconds(0);
	today.setSecond(0);
	today.setMinute(0);
	today.setHour(0);
	
	const long minTimestamp = today.timestamp();
	const long maxTimestamp = minTimestamp + 60L*60L*24L*1000L;
	
	return getPoints(db, node, minTimestamp, maxTimestamp, limit, offset);
}

vector<DataPoint> getPointsLastWeek(SQLite3Db &db, const long node, const long limit = 1000, const long offset = 0L) {
	lazy::DateTime today;
	today.setMilliseconds(0);
	today.setSecond(0);
	today.setMinute(0);
	today.setHour(0);
	
	const long maxTimestamp = today.timestamp() + 60L*60L*24L*1000L;
	const long minTimestamp = maxTimestamp - 60L*60L*24L*1000L*7L;
	
	return getPoints(db, node, minTimestamp, maxTimestamp, limit, offset);
}

vector<DataPoint> getPointsLastMonth(SQLite3Db &db, const long node, const long limit = 1000, const long offset = 0L) {
	lazy::DateTime today;
	today.setMilliseconds(0);
	today.setSecond(0);
	today.setMinute(0);
	today.setHour(0);
	
	const long maxTimestamp = today.timestamp() + 60L*60L*24L*1000L;
	const long minTimestamp = maxTimestamp - 60L*60L*24L*1000L*31L;
	
	return getPoints(db, node, minTimestamp, maxTimestamp, limit, offset);
}


/** Extract parameters to a parameter map */
static map<String, String> extractParams(String param) {
	vector<String> split = param.split("&");
	
	map<String, String> ret;
	for(vector<String>::const_iterator it = split.begin(); it != split.end(); ++it) {
		String t = (*it);
		size_t index = t.find('=');
		if(index == string::npos)
			ret[t] = "";
		else {
			String name = t.left(index);
			String value = t.mid(index+1);
			ret[name] = value;
		}
	}
	return ret;
}

void Webserver::doHttpRequest(const int fd) {
	SQLite3Db db(this->db_filename);		// Open database
	
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
	
	
			
	try {
	
		// Switch requests uris
		if(url == "/" || url == "/index.html" || url == "/index.html") {
	
		    socket.writeHttpHeader();
		    socket << "<html><head><title>meteo Sensor node</title></head>";
		    socket << "<body>";
		    socket << "<h1>Meteo Server</h1>\n";
			socket << "<p><a href=\"index.html\">[Nodes]</a></p>";
			// Get nodes
			vector<Node> nodes = getNodes(db);
			if(nodes.size() == 0) {
				socket << "No nodes present in the system";
			} else {
				socket << "<table border=\"1\">\n";
				socket << "<tr><td><b>Node</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [% rel]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminosity (visible)</b></td><td><b>Luminosity (IR)</b></td></tr>\n";
				for(vector<Node>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
					socket << "<tr><td><a href=\"Node?id=" << (*it).id << "\">" << (*it).name << "</a></td>";
					vector<DataPoint> dp = getPoints(db, (*it).id, -1,-1, 1);
					if(dp.size() > 0) {
						DataPoint p = dp[0];
						socket << "<td>" << p.t << "</td><td>" << p.hum << "</td><td>" << p.p << "</td><td>" << p.l_vis << "</td><td>" << p.l_ir << "</td>";
					}
					socket << "</tr>\n";
				}
				socket << "</table>\n";
			}
		
		
		} else if(url.startsWith("/api/")) {
		
			String apiRequest = url.mid(5);
		
			if(apiRequest == "nodes") {
				socket.writeHttpHeader(200);
			
				socket << "# Node ID, Node Name\n";
			
				vector<Node> nodes = getNodes(db);
				for(vector<Node>::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
					socket << (*it).id << "," << (*it).name << "\n";
			
			} else if(apiRequest.startsWith("node?")) {
			
				map<String, String> params = extractParams(apiRequest.mid(5));
				long id = params["id"].toLong();
			
				long limit = 1000;
				long offset = 0;
			
				// Default timestamp: today
				lazy::DateTime today;
				today.setMilliseconds(0);
				today.setSecond(0);
				today.setMinute(0);
				today.setHour(0);
				long minTimestamp = today.timestamp();
				long maxTimestamp = minTimestamp + 60L*60L*24L*1000L;
			
				vector<DataPoint> dp = getPoints(db, id, minTimestamp, maxTimestamp, limit, offset);
				
				socket.writeHttpHeader(200);
				socket << "# Timestamp, Temperature, Humditiy, Pressure, Light (IR/Visible)\n";
				socket << "# [Seconds since Epoc], [deg C], [% rel], [hPa], [1]\n";
				socket << "# COUNT=" << (long)dp.size() << "\n";
				for( vector<DataPoint>::const_iterator it = dp.begin(); it != dp.end(); ++it) {
					socket << (*it).timestamp << ", ";
					socket << (*it).t << ", ";
					socket << (*it).hum << ", ";
					socket << (*it).p << ", ";
					socket << (*it).l_ir << '/' << (*it).l_vis << '\n';
				}
			
			} else {
		    	socket.writeHttpHeader(404);
		    	socket << "Illegal API request\n";
		    }
		
		} else if(url == "/Node" || url == "/node") {
		
		    socket.writeHttpHeader(403);
		    socket << "<html><head><title>meteo Sensor node</title></head>";
		    socket << "<body>";
		    socket << "<h1>Meteo Server</h1>\n";
			socket << "<p><a href=\"index.html\">[Nodes]</a></p>";
			socket << "<b>Error</b>Missing parameter: Node id\n";
		
		} else if(url.startsWith("/Node?") || url.startsWith("/node?")) {
			map<String, String> params = extractParams(url.mid(6));
		
			String sID = params["id"];
			long id = sID.toLong();
		
		    socket.writeHttpHeader();
		    socket << "<html><head><title>meteo Sensor node</title></head>";
		    socket << "<body>";
		    socket << "<h1>Meteo Server</h1>\n";
			socket << "<p><a href=\"index.html\">[Nodes]</a></p>";
			socket << "<p><a href=\"Node?id=" << id << "&type=day\">[Today]</a> <a href=\"Node?id=" << id << "&type=week\">[7 days]</a> <a href=\"Node?id=" << id << "&type=month\">[Month]</a></p>";
		
			vector<DataPoint> datapoints;
		
			String type = params["type"];
			string dt_format = "%H:%M:%S";
			if(type == "day" || type == "") {
				datapoints = getPointsToday(db, id);
			} else if(type == "week") {
				datapoints = getPointsLastWeek(db, id);
			} else if(type == "month") {
				datapoints = getPointsLastMonth(db, id);
			} else {
				// Custom range?
			
			}
		
			// Print datapoints
			socket << "<table border=\"1\">\n";
			socket << "<tr><td><b>Timestamp</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [% rel]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminosity (visible)</b></td><td><b>Luminosity (IR)</b></td></tr>\n";
			for(vector<DataPoint>::const_iterator it = datapoints.begin(); it != datapoints.end(); ++it) {
				DataPoint p = (*it);
				lazy::DateTime dt(p.timestamp);
				socket << "<td>" << dt.format(dt_format) << "</td><td>" << p.t << "</td><td>" << p.hum << "</td><td>" << p.p << "</td><td>" << p.l_vis << "</td><td>" << p.l_ir << "</td></tr>\n";
			}
			socket << "</table>\n";
		
		
		} else {
			// Not found
			socket << "HTTP/1.1 404 Not Found\n";
			socket << "Content-Type: text/html\n\n";
			socket << "<html><head><title>Not found</title></head><body><h1>Not found</h1>";
			socket << "<p>Error 404 - Page not found. Maybe you want to <a href=\"index.html\">go back to the homepage</a></p>\n";
			socket << "<p><b>Illegal url:</b>" << url << "</p>\n";
		}
	
	} catch (SQLException &e) {
		socket.writeHttpHeader(500);
		socket << "SQL Error : " << e.what() << "\n";
		cerr << "SQL error in HTTP request: " << e.what() << endl;
	}
	
}
