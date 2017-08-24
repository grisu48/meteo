/* =============================================================================
 * 
 * Title:         meteod - Meteo data collector server
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Collects data via mosquitto interface and writes them to a
 *                SQLite3 database. Provides HTTP access to clients
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <cstdlib>
#include <vector>

#include <unistd.h>
#include <signal.h>

#include "config.hpp"
#include "json.hpp"
#include "mosquitto.hpp"
#include "string.hpp"
#include "time.hpp"
#include "collector.hpp"

#include "httpd.cpp"

using namespace std;
using flex::String;
using json = nlohmann::json;
using namespace lazy;


/** Mosquitto instances */
static vector<Mosquitto*> mosquittos;
static Collector collector;
static volatile bool running = true;


/** Mosquitto receive callback */
static void mosq_receive(const std::string &topic, char* buffer, size_t len);
/** Connect a mosquitto server */
static Mosquitto* connectMosquitto(string remote, int port = 1883, string username = "", string password = "");
/** Program cleanup */
static void cleanup();
/** Signal handler */
static void sig_handler(int signo);
/** Start and run www_server on the given port. This call shouldn't return */
static void http_server_run(const int port);


int main() { //int argc, char** argv) {
	const char* db_filename = "meteod.db";
	const char* mosq_server = "localhost";
	int http_port = 8900;
	// Open database
	collector.open_db(db_filename);
	
    try {
    	connectMosquitto(mosq_server);
    } catch (const char* msg) {
    	cerr << "Error connecting to mosquitto remote: " << msg << endl;
    	return EXIT_FAILURE;
    }
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGUSR1, sig_handler);
    atexit(cleanup);
    collector.start();
    
    #if 0
    while(true) {
   		vector<Station> stations = collector.activeStations();
   		
   		for(vector<Station>::iterator it = stations.begin(); it != stations.end(); ++it) {
			try {
				vector<DataPoint> dp = collector.query((*it).id,-1,-1,1);
				if(dp.size() > 0) {
					cout << (*it).name << " - " << dp[0].t << " deg C, " << dp[0].hum << " % rel, " << dp[0].p << " hPa" << endl;
				}
			} catch (const char* msg) {
				cerr << msg << endl;
			}
   		}
	    sleep(5);
    }
    #endif
    
    // Setup http server
    http_server_run(http_port);
    
    return EXIT_SUCCESS;
}



static void mosq_receive(const std::string &topic, char* buffer, size_t len) {
	(void)len;
	(void)topic;
	
	// Parse the packet
	try {
		string packet(buffer);
		json j = json::parse(packet);
	
		if (j.find("node") == j.end()) return;
		const long id = j["node"].get<long>();		
		string name = "";
		float t = 0.0F;
		float hum = 0.0F;
		float p = 0.0F;
		float l_vis = 0.0F;
		float l_ir = 0.0F;
		
		if (j.find("name") != j.end()) name = j["name"].get<string>();
		if (j.find("t") != j.end()) t = j["t"].get<float>();
		if (j.find("hum") != j.end()) hum = j["hum"].get<float>();
		if (j.find("p") != j.end()) p = j["p"].get<float>();
		if (j.find("l_vis") != j.end()) l_vis = j["l_vis"].get<float>();
		if (j.find("l_ir") != j.end()) l_ir = j["l_ir"].get<float>();
		
		collector.push(id, name, t, hum, p, l_vis, l_ir);
	
	} catch (std::exception &e) {
		cerr << "Exception (" << e.what() << ") parsing packet" << endl << buffer << endl;
	} catch (...) {
		cerr << "Unknown error parsing packet " << buffer << endl;
		return;
	}
}

static Mosquitto* connectMosquitto(string remote, int port, string username, string password) {
	Mosquitto *mosq = new Mosquitto(remote, port);
	if(username.size() > 0 || password.size() > 0) {
		mosq->setUsername(username);
		mosq->setPassword(password);
	}
	
	try {
		mosq->connect();
		mosq->setReceiveCallback(mosq_receive);
		
		mosq->subscribe("meteo/#");
		mosq->start();
	} catch (...) {
		delete mosq;
		throw;
	}
	mosquittos.push_back(mosq);
	return mosq;
}


static void cleanup() {
	cerr << "Cleanup ... " << endl;
	collector.close();
	
	for(vector<Mosquitto*>::iterator it = mosquittos.begin(); it != mosquittos.end(); ++it)
		delete *it;
	mosquittos.clear();
}


static void sig_handler(int signo) {
	switch(signo) {
		case SIGINT:
		case SIGTERM:
			exit(EXIT_FAILURE);
			return;
	}
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

static void* http_thread_run(void *arg) {
	Socket *socket = (Socket*)arg;
	
	// Read request
	String line;
	vector<String> header;
	while(true) {
		line = socket->readLine();
		line = line.trim();
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
		*socket << "Unsupported protocol";
		delete socket;
		return NULL;
	}
	
	try {
		
		// Switch requests uris
		if(url == "/" || url == "/index.html" || url == "/index.html") {
	
		    socket->writeHttpHeader();
		    *socket << "<html><head><title>meteo Sensor node</title>";
		    *socket << "<meta http-equiv=\"refresh\" content=\"5\"></head>";
		    *socket << "<body>";
		    *socket << "<h1>Meteo Server</h1>\n";
			*socket << "<p><a href=\"index.html\">[Nodes]</a></p>\n";
			*socket << "</html>";
			
			vector<Station> stations = collector.activeStations();
			*socket << "<table border=\"1\">";
			*socket << "<tr><td><b>Stations</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [rel %]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminositry</b></td>\n";
			for( vector<Station>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
				*socket << "<tr><td><a href=\"node?id=" << (*it).id << "\">" << (*it).name << "</a></td>";
				*socket << "<td>" << (*it).t << "</td>";
				*socket << "<td>" << (*it).hum << "</td>";
				*socket << "<td>" << (*it).p << "</td>";
				*socket << "<td>" << (*it).l_ir << "/" << (*it).l_vis << "</td>";
				*socket << "</tr>";
			}
			*socket << "</table>";
			
		} else if(url.startsWith("/Node?") || url.startsWith("/node?")) {
			
			map<String, String> params = extractParams(url.mid(6));
			
			try {
				long id = 0L;	
				if(params.find("id") == params.end()) throw "id not found";
				id = ::atol(params["id"].c_str());
				long minTimestamp = -1L;
				long maxTimestamp = -1L;
				long limit = 1000L;
				long offset = 0L;
				String format = "html";
				if(params.find("format") != params.end()) format = params["format"];
				if(params.find("t_min") != params.end()) minTimestamp = ::atol(params["t_min"].c_str());
				if(params.find("t_max") != params.end()) maxTimestamp = ::atol(params["t_max"].c_str());
				if(params.find("limit") != params.end()) limit = ::atol(params["limit"].c_str());
				if(params.find("offset") != params.end()) offset = ::atol(params["offset"].c_str());
				if(limit < 0) limit = 100;
				if(limit > 1000) limit = 1000;
				if(offset < 0) offset = 0L;
				
				vector<DataPoint> dp = collector.query(id, minTimestamp, maxTimestamp, limit, offset);
				
				if(format == "" || format == "html") {
					socket->writeHttpHeader();
										
					*socket << "<html><head><title>meteo Sensor node</title></head>";
					*socket << "<body>";
					*socket << "<h1>Meteo Server</h1>\n";
					*socket << "<p><a href=\"index.html\">[Nodes]</a></p>\n";
					*socket << "</html>";
			
					vector<Station> stations = collector.activeStations();
					*socket << "<table border=\"1\">";
			*socket << "<tr><td><b>Timestamp</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [rel %]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminositry</b></td>\n";
					
					for(vector<DataPoint>::const_iterator it = dp.begin(); it != dp.end(); ++it) {
						*socket << "<tr><td>" << (*it).timestamp << "</td>";
						*socket << "<td>" << (*it).t << "</td>";
						*socket << "<td>" << (*it).hum << "</td>";
						*socket << "<td>" << (*it).p << "</td>";
						*socket << "<td>" << (*it).l_ir << "/" << (*it).l_vis << "</td>";
						*socket << "</tr>";
					}
					*socket << "</table>";
				} else if(format == "csv") {
					for(vector<DataPoint>::const_iterator it = dp.begin(); it != dp.end(); ++it) {
						*socket << (*it).timestamp << ", ";
						*socket << (*it).t << ", ";
						*socket << (*it).hum << ", ";
						*socket << (*it).p << ",";
						*socket << (*it).l_ir << "/" << (*it).l_vis << "\n";
					}
				} else {
					socket->writeHttpHeader(400);
					*socket << "Illegal format";
				}
			
			} catch (const char* msg) {
				socket->writeHttpHeader(500);
				*socket << msg;
			}
			
			
		} else if(url == "/nodes" || url == "/Nodes" || url.startsWith("/Nodes?") || url.startsWith("/nodes?")) {
			
			
			map<String, String> params;
			if(url.size() > 6) params = extractParams(url.mid(7));
			
			String format = "html";
			if(params.find("format") != params.end()) format = params["format"];
			
				
			if(format == "" || format == "html") {
			
				socket->writeHttpHeader();
				*socket << "<html><head><title>meteo Sensor node</title></head>";
				*socket << "<body>";
				*socket << "<h1>Meteo Server</h1>\n";
				*socket << "<p><a href=\"index.html\">[Nodes]</a></p>\n";
				*socket << "</html>";
			
				*socket << "<table border=\"1\">";
				*socket << "<tr><td><b>Stations</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [rel %]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminositry</b></td>\n";
				vector<Station> stations = collector.activeStations();
				for( vector<Station>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
					*socket << "<tr><td><a href=\"node?id=" << (*it).id << "\">" << (*it).name << "</a></td>";
					*socket << "<td>" << (*it).t << "</td>";
					*socket << "<td>" << (*it).hum << "</td>";
					*socket << "<td>" << (*it).p << "</td>";
					*socket << "<td>" << (*it).l_ir << "/" << (*it).l_vis << "</td>";
					*socket << "</tr>";
				}
				*socket << "</table>";
			} else if(format == "csv") {
			
				socket->writeHttpHeader();
				vector<Station> stations = collector.activeStations();
				for( vector<Station>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
					*socket << (*it).id << "," << (*it).name << "\n";
				}
			
			
			} else {
				socket->writeHttpHeader(400);
				*socket << "Illegal format";
			}
			
		} else {
			socket->writeHttpHeader(404);
			*socket << "404 - Not found";
		}
		
	} catch (...) {
		delete socket;
		cerr << "Unknown error in http request" << endl;
		return NULL;
	}
	
	// Done
	delete socket;
	return NULL;
}

static void http_server_run(const int port) {
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
	
		// Server loop
		while(running) {
			const int fd = ::accept(sock, NULL, 0);
		
			// Socket will be destroyed by thread
			Socket *socket = new Socket(fd);
			// Create new thread for the socket
			pthread_t tid;
			if( pthread_create(&tid, NULL, http_thread_run, socket) < 0) throw "Error creating new http thread";
			pthread_detach(tid);
		}
	
		::close(sock);
	} catch(...) {
		::close(sock);
		throw;
	}
}
