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


void Webserver::loop(void) {
	while(running && this->sock > 0) {
		const int fd = ::accept(sock, NULL, 0);
		if(fd < 0) break;		// Terminate
		
		const pid_t pid = fork();
		if(pid < 0) {
			throw "Cannot fork";
			::close(sock);
			exit(EXIT_FAILURE);
		} else if(pid == 0) {
			// Do http request
			::close(sock);
			this->doHttpRequest(fd);
			::close(fd);
			exit(EXIT_SUCCESS);
		} else {
			::close(fd);
			int status;
			// Wait for child
			waitpid(pid, &status, 0);
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
		node.print();
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
	//cout << str << " = ";
	
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
	//cout << ret.size() << endl;
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
			socket << "<ul>\n";
			for(vector<Node>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
				socket << "<li><a href=\"Node?id=" << (*it).id << "\">" << (*it).name << "</a>";
				vector<DataPoint> dp = getPoints(db, (*it).id, -1,-1, 1);
				if(dp.size() > 0) {
					DataPoint p = dp[0];
					socket << " (" << p.t << " deg C, " << p.hum << " % rel., " << p.p << " hPa)";
				}
				socket << "</li>\n";
			}
			socket << "</ul>\n";
		}
		
	} else {
		// Not found
		socket << "HTTP/1.1 404 Not Found\n";
		socket << "Content-Type: text/html\n\n";
		socket << "<html><head><title>Not found</title></head><body><h1>Not found</h1>";
		socket << "<p>Error 404 - Page not found. Maybe you want to <a href=\"index.html\">go back to the homepage</a></p>";
	}
}
