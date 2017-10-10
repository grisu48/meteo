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
#include <sys/types.h>
#include <signal.h>
#include <math.h>
#include <sys/socket.h>
#include <execinfo.h>

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

#define VERSION "0.2.4"
#define BUILD 240


/** Mosquitto instances */
static vector<Mosquitto*> mosquittos;
static Collector collector;
static volatile bool running = true;
static DateTime startupDateTime;
static int verbose = 0;

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
static void fork_daemon(void);
static void backtrace();
static void terminateHandler();


int main(int argc, char** argv) {
	const char* db_filename = ":memory:";   // Default database in memory
	const char* mosq_server = "localhost";
	int http_port = 8900;
	int uid = 0;		// UID or 0, if not set
	int gid = 0;		// GID or 0, if not set
	bool daemon = false;	// Daemon mode
		
	cout << "meteod - Meteo server | 2017, Felix Niederwanger" << endl;
	cout << "  Version " << VERSION << " (Build " << BUILD << ")" << endl;
	
	// Parse program arguments
	try {
		for(int i=1;i<argc;i++) {
			string arg(argv[i]);
			if(arg.size() == 0) continue;
			if(arg.at(0) == '-') {
				if(arg == "-h" || arg == "--help") {
					cout << endl;
					cout << "Usage: " << argv[0] << " [OPTIONS] [REMOTE]" << endl;
					cout << "OPTIONS" << endl;
					cout << "  -h       --help                     Print this help message" << endl;
					cout << "           --http PORT                Set PORT as webserver port" << endl;
					cout << "  -f FILE  --db FILE                  Set FILE as Sqlite3 database" << endl;
					cout << "  -d       --daemon                   Daemon mode" << endl;
					cout << "           --uid UID                  Set UID" << endl;
					cout << "           --gid GID                  Set group id (GID)" << endl;
					cout << "  -v       --verbose                  Verbose run" << endl;
					cout << "           --delay SECONDS            Delay between write cycles in seconds" << endl;
					cout << "  -vv                                 Verbose run including http requests" << endl;
					cout << "REMOTE is the mosquitto server" << endl;
					exit(EXIT_SUCCESS);
				
				} else if(arg == "-f" || arg == "--db") {
					if(i >= argc-1) throw "Missing argument: Database";
					db_filename = argv[++i];
				} else if(arg == "-d" || arg == "--daemon") {
					daemon = true;
				} else if(arg == "-v" || arg == "--verbose") {
					verbose = 1;
				} else if(arg == "-vv") {
					verbose = 2;
				} else if(arg == "--delay") {
					if(i >= argc-1) throw "Missing argument: Delay";
					int delay = ::atoi(argv[++i]);
					if(delay <= 0) throw "Invalid delay";
					collector.delay(delay);
				} else if(arg == "--http") {
					if(i >= argc-1) throw "Missing argument: Http port";
					http_port = ::atoi(argv[++i]);
					if(http_port <= 0 || http_port > 65535) throw "Illegal port";
				} else if(arg == "--uid") {
					if(i >= argc-1) throw "Missing argument: UID";
					uid = ::atoi(argv[++i]);
				} else if(arg == "--gid") {
					if(i >= argc-1) throw "Missing argument: UID";
					gid = ::atoi(argv[++i]);
				} else {
					cerr << "Illegal argument: " << arg << endl;
					exit(EXIT_FAILURE);
				}		
			} else {
				mosq_server = argv[i];
			}
		}
	} catch (const char* msg) {
		cerr << "Error: " << msg << endl;
		exit(EXIT_FAILURE);
	}
	collector.verbose(verbose);
	
	// Daemon and set gid/uid
	if(daemon) fork_daemon();
	if(gid > 0 && setgid(gid) < 0) {
		cerr << "Error setting gid " << gid << ": " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
	if(uid > 0 && setuid(uid) < 0) {
		cerr << "Error setting uid " << uid << ": " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
	
	if( (uid > 0 || gid > 0) && verbose > 1)
		cout << "Permissions dropped. uid=" << getuid() << " (" << geteuid() << "), gid = " << getgid() << endl;
	
	
	// Open database
	if(verbose > 0) cout << "Database file: " << db_filename << endl;
	try {
		collector.open_db(db_filename);
    } catch (const char* msg) {
    	cerr << "Error opening database: " << strerror(errno) << endl;
    	exit(EXIT_FAILURE);
    }
	
    try {
    	if(verbose > 0) cout << "Connecting to " << mosq_server << " ... " << endl;
    	connectMosquitto(mosq_server);
    } catch (const char* msg) {
    	cerr << "Error connecting to mosquitto remote: " << msg << endl;
    	exit(EXIT_FAILURE);
    }
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGABRT, sig_handler);
    atexit(cleanup);
    std::set_terminate(terminateHandler);
    collector.start();
    
    // Setup http server
    if(verbose > 0) cout << "Setting up http server on port " << http_port << " ... " << endl;
    try {
	    http_server_run(http_port);
    } catch (const char* msg) {
    	cerr << "Http server error: " << msg << endl;
    	exit(EXIT_FAILURE);
    }
    
    // Should never happen
    cout << "Bye" << endl;
    return EXIT_SUCCESS;
}


Lightning parseLightning(const std::string packet) {
	json j = json::parse(packet);
	
	Lightning ret;
	
	//buf << "{\"station\":" << node_id << ",\"timestamp\":" << timestamp << ",\"distance\":" << distance << "}";
	
	if (j.find("station") == j.end()) throw "No station";
	if (j.find("timestamp") == j.end()) throw "No timestamp";
	if (j.find("distance") == j.end()) throw "No distance";
	ret.station = j["station"].get<long>();	
	ret.timestamp = j["timestamp"].get<long>();	
	ret.distance = j["distance"].get<float>();	
	return ret;
}

static void mosq_receive(const std::string &s_topic, char* buffer, size_t len) {
	(void)len;
	String topic = s_topic;
	
	if(topic.startsWith("lightning/")) {
		// Lightning packet
		String packet(buffer);
		
		try {
			
			Lightning lightning = parseLightning(packet);
			if(verbose > 1) cerr << "LIGHTNING RECEIVED: " << lightning.toString() << endl;
			collector.push(lightning);
			
		} catch (const char* msg) {
			cerr << "Error parsing lightning packet: " << msg << endl;
		}
		
	} else if (topic.startsWith("meteo/")) {
		// Meteo packet 
	
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
			
			if (verbose > 1) cout << "meteo[" << id << ",\"" << name << "\"] = " << t << " deg C, " << hum << " % rel, " << p << " kPa, " << l_vis << "/" << l_ir << " light" << endl;
			
			collector.push(id, name, t, hum, p, l_vis, l_ir);
	
		} catch (std::exception &e) {
			cerr << "Exception (" << e.what() << ") parsing packet" << endl << buffer << endl;
		} catch (...) {
			cerr << "Unknown error parsing packet " << buffer << endl;
			return;
		}
	} else {
		cerr << "Illegal topic: " << topic << endl;
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
		mosq->subscribe("lightning/#");
		mosq->start();
	} catch (...) {
		delete mosq;
		throw;
	}
	mosquittos.push_back(mosq);
	return mosq;
}


static void cleanup() {
	if(verbose > 0) cerr << "Cleanup ... " << endl;
	collector.close();
	
	for(vector<Mosquitto*>::iterator it = mosquittos.begin(); it != mosquittos.end(); ++it)
		delete *it;
	mosquittos.clear();
}


static void sig_handler(int signo) {
	switch(signo) {
		case SIGINT:
			if(verbose > 0) {
				DateTime now;
				cerr << "[" << now.format() << "] SIGINT received" << endl;
			}
			exit(EXIT_FAILURE);
			return;
		case SIGTERM:
			if(verbose > 0) {
				DateTime now;
				cerr << "[" << now.format() << "] SIGTERM received" << endl;
			}
			exit(EXIT_FAILURE);
			return;
		case SIGUSR1:
			{
				// Print current readings
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
		   		return;
	   		}
		case SIGSEGV:
			cerr << "Segmentation fault" << endl;
			backtrace();
			exit(EXIT_FAILURE);
			return;
		case SIGABRT:
			cerr << "Abnormal program termination" << endl;
			backtrace();
			exit(EXIT_FAILURE);
			return;
	}
}

static void fork_daemon(void) {
	pid_t pid = fork();
	if(pid < 0) {
		cerr << "Fork daemon failed" << endl;
		exit(EXIT_FAILURE);
	} else if(pid > 0) {
		// Success. The parent leaves here
		exit(EXIT_SUCCESS);
	}
	
	/* Fork off for the second time */
	/* This is needed to detach the deamon from a terminal */
	pid = fork();
	if(pid < 0) {
		cerr << "Fork daemon failed (step two)" << endl;
		exit(EXIT_FAILURE);
	} else if(pid > 0) {
		// Success. The parent again leaves here
		exit(EXIT_SUCCESS);
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

static float round_f(const float x, const int digits = 2) {
	const float base = (float)::pow10(digits);
	return ::roundf(x * base)/base;
}

static void* http_thread_run(void *arg) {
	Socket *socket = (Socket*)arg;
	
	
	try {
	
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
		if(! (protocol.equalsIgnoreCase("HTTP/1.1") || protocol.equalsIgnoreCase("HTTP/1.0"))) {
			*socket << "Unsupported protocol";
			delete socket;
			return NULL;
		}
	
		if(verbose > 1) {
			DateTime now;
		
			cout << "[" << now.format() << "] " << socket->getRemoteAddress() << " " << protocol << " " << url << endl;
		}
	
		
		// Switch requests uris
		if(url == "/" || url == "/index.html" || url == "/index.html") {
	
		    socket->writeHttpHeader();
		    *socket << "<html><head><title>meteo Server</title>";
		    *socket << "<meta http-equiv=\"refresh\" content=\"5\"></head>";
		    *socket << "<body>";
		    *socket << "<h1>Meteo</h1>\n";
			*socket << "<p><a href=\"index.html\">[Nodes]</a> <a href=\"current?format=html\">[Current]</a></p>\n";
		    *socket << "<p>Meteo server v" << VERSION << " (Build " << BUILD << ") -- 2017, Felix Niederwanger<br/>";
		    *socket << "Server started: " << startupDateTime.format() << "</p>";
		    
		    *socket << "<h2>Overview</h2>\n";
			
			vector<Station> stations = collector.activeStations();
			*socket << "<table border=\"1\">";
			*socket << "<tr><td><b>Stations</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [rel %]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminositry</b></td>\n";
			for( vector<Station>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
				*socket << "<tr><td><a href=\"node?id=" << (*it).id << "\">" << (*it).name << "</a></td>";
				*socket << "<td>" << round_f((*it).t) << "</td>";
				*socket << "<td>" << round_f((*it).hum) << "</td>";
				*socket << "<td>" << round_f((*it).p) << "</td>";
				*socket << "<td>" << round_f((*it).l_ir) << "/" << round_f((*it).l_vis) << "</td>";
				*socket << "</tr>";
			}
			*socket << "</table>";
			
			DateTime now;
			*socket << "</p>" << now.format() << "</p>";
			
		} else if(url.startsWith("/Lightnings?") || url.startsWith("/lightnings?")) {
			
			map<String, String> params = extractParams(url.mid(12));
			
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
				if(limit > 10000) limit = 10000;
				if(offset < 0) offset = 0L;
			
				vector<Lightning> dp = collector.query_lightnings(id, minTimestamp, maxTimestamp, limit, offset);	
				
				if(format == "" || format == "html") {
					socket->writeHttpHeader();
										
					*socket << "<html><head><title>meteo Server</title></head>";
					*socket << "<body>";
					*socket << "<h1>Meteo</h1>\n";
					*socket << "<p><a href=\"index.html\">[Nodes]</a> <a href=\"current?format=html\">[Current]</a></p>\n";
					
					*socket << "<h3>Lightnings</h3>\n";
					
					if(dp.size() == 0) {
						*socket << "<p>No lightnings fetched</p>\n";
					
					} else {
					
						*socket << "<p>" << dp.size() << " lightnings</p>\n";
					
						*socket << "<table border=\"1\">";
						*socket << "<tr><td><b>Timestamp</b></td><td><b>Distance [km]</b></td></tr>\n";
					
						lazy::DateTime dateTime;
						for(vector<Lightning>::const_iterator it = dp.begin(); it != dp.end(); ++it) {
							dateTime.timestamp((*it).timestamp*1000L);
							*socket << "<tr><td>" << dateTime.format() << "</td>";
							*socket << "<td>" << round_f((*it).distance) << "</td>";
							*socket << "</tr>";
						}
						*socket << "</table>";
					
					}
					
				} else if(format == "csv" || format == "plain") {
				
					socket->writeHttpHeader();
					for(vector<Lightning>::const_iterator it = dp.begin(); it != dp.end(); ++it) {
						*socket << (*it).timestamp << ", ";
						*socket << round_f((*it).distance) << "\n";
					}
				} else {
					socket->writeHttpHeader(400);
					*socket << "Illegal format";
				}
			
			} catch (const char* msg) {
				socket->writeHttpHeader(500);
				*socket << msg;
			}
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
				if(limit > 10000) limit = 10000;
				if(offset < 0) offset = 0L;
				
				vector<DataPoint> dp = collector.query(id, minTimestamp, maxTimestamp, limit, offset);
				
				Station station = collector.station(id);
				
				if(format == "" || format == "html") {
					socket->writeHttpHeader();
										
					*socket << "<html><head><title>meteo Server</title></head>";
					*socket << "<body>";
					*socket << "<h1>Meteo</h1>\n";
					*socket << "<p><a href=\"index.html\">[Nodes]</a> <a href=\"current?format=html\">[Current]</a></p>\n";
					string name = station.name;
					if(name == "") name = "UNKNOWN";
					*socket << "<h3>Station overview: " << name << "</h3>\n";
					
					// History
					int64_t timestamp = getSystemTime()/1000L;
					*socket << "<p>Display: ";
					*socket << "<a href=\"node?id=" << id << "&t_min=" << (timestamp-3*60*60L) << "&t_max=" << timestamp << "\">[3h]</a> ";
					*socket << "<a href=\"node?id=" << id << "&t_min=" << (timestamp-12*60*60L) << "&t_max=" << timestamp << "\">[12h]</a> ";
					*socket << "<a href=\"node?id=" << id << "&t_min=" << (timestamp-24*60*60L) << "&t_max=" << timestamp << "\">[24h]</a> ";
					*socket << "<a href=\"node?id=" << id << "&t_min=" << (timestamp-48*60*60L) << "&t_max=" << timestamp << "\">[48h]</a> ";
					*socket << "<a href=\"node?id=" << id << "&t_min=" << (timestamp-7L*24L*60L*60L) << "&t_max=" << timestamp << "\">[7d] </a>";
					*socket << "<a href=\"node?id=" << id << "&t_min=" << (timestamp-30L*24L*60L*60L) << "&t_max=" << timestamp << "\">[30d]</a>";
					*socket << "</p>\n";
					
					DataPoint avg;
					for(vector<DataPoint>::const_iterator it = dp.begin(); it != dp.end(); ++it) {
						avg.t += (*it).t;
						avg.hum += (*it).hum;
						avg.p += (*it).p;
						avg.l_vis += (*it).l_vis;
						avg.l_ir += (*it).l_ir;
					}
					if(dp.size() > 0) {
						const float n = (float)dp.size();
						avg.t /= n;
						avg.hum /= n;
						avg.p /= n;
						avg.l_vis /= n;
						avg.l_ir /= n;
					}
					
					lazy::DateTime dateTime;
					*socket << "<table border=\"1\">";
					*socket << "<tr><td>Values</td><td>" <<(long)dp.size() << "</td></tr>\n";
					*socket << "<tr><td>Time range</td><td>";
					if(dp.size() == 0) {
						*socket << "-";
					} else if(dp.size() == 1) {
						dateTime.timestamp(dp[0].timestamp*1000L); *socket << dateTime.format();
					} else {
						dateTime.timestamp(dp[dp.size()-1].timestamp*1000L); *socket << dateTime.format() << " - ";
						dateTime.timestamp(dp[0].timestamp*1000L); *socket << dateTime.format();
					}
					*socket << "</td></tr>\n";
					*socket << "<tr><td>Temperature</td><td>" << avg.t << " deg C</td></tr>\n";
					*socket << "<tr><td>Humidity</td><td>" << avg.hum << " % rel</td></tr>\n";
					*socket << "<tr><td>Pressure</td><td>" << avg.p << " hPa</td></tr>\n";
					*socket << "<tr><td>Luminosity</td><td>" << avg.l_ir << "/" << avg.l_vis << "</td></tr>\n";
					*socket << "</table>";
					
					*socket << "<h3>Data</h3>\n";
					*socket << "<p><a href=\"node?id=" << id << "&t_min=" << minTimestamp << "&t_max=" << maxTimestamp << "&format=csv\">[Show CSV]</a></p>\n";
					*socket << "<table border=\"1\">";
					*socket << "<tr><td><b>Timestamp</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [rel %]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminositry</b></td></tr>\n";
					
					for(vector<DataPoint>::const_iterator it = dp.begin(); it != dp.end(); ++it) {
						dateTime.timestamp((*it).timestamp*1000L);
						*socket << "<tr><td>" << dateTime.format() << "</td>";
						*socket << "<td>" << round_f((*it).t) << "</td>";
						*socket << "<td>" << round_f((*it).hum) << "</td>";
						*socket << "<td>" << round_f((*it).p) << "</td>";
						*socket << "<td>" << round_f((*it).l_ir) << "/" << (*it).l_vis << "</td>";
						*socket << "</tr>";
					}
					*socket << "</table>";
					
					DateTime now;
					*socket << "</p>" << now.format() << "</p>";
					
					*socket << "</html>";
				} else if(format == "csv" || format == "plain") {
				
					socket->writeHttpHeader();
					for(vector<DataPoint>::const_iterator it = dp.begin(); it != dp.end(); ++it) {
						*socket << (*it).timestamp << ", ";
						*socket << round_f((*it).t) << ", ";
						*socket << round_f((*it).hum) << ", ";
						*socket << round_f((*it).p) << ",";
						*socket << round_f((*it).l_ir) << "/" << (*it).l_vis << "\n";
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
				*socket << "<html><head><title>meteo Server</title></head>";
				*socket << "<body>";
				*socket << "<h1>Meteo Server</h1>\n";
				*socket << "<p><a href=\"index.html\">[Nodes]</a> <a href=\"current?format=html\">[Current]</a></p>\n";
				*socket << "</html>";
				
				*socket << "<table border=\"1\">";
				*socket << "<tr><td><b>Stations</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [rel %]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminositry</b></td></tr>\n";
				vector<Station> stations = collector.activeStations();
				for( vector<Station>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
					*socket << "<tr><td><a href=\"node?id=" << (*it).id << "\">" << (*it).name << "</a></td>";
					*socket << "<td>" << round_f((*it).t) << "</td>";
					*socket << "<td>" << round_f((*it).hum) << "</td>";
					*socket << "<td>" << round_f((*it).p) << "</td>";
					*socket << "<td>" << round_f((*it).l_ir) << "/" << (*it).l_vis << "</td>";
					*socket << "</tr>";
				}
				*socket << "</table>";
			} else if(format == "csv" || format == "plain") {
			
				socket->writeHttpHeader();
				vector<Station> stations = collector.activeStations();
				for( vector<Station>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
					*socket << (*it).id << "," << (*it).name << "\n";
				}
			
			
			} else {
				socket->writeHttpHeader(400);
				*socket << "Illegal format";
			}
			
		} else if(url.startsWith("/current") || url.startsWith("/current?")) {
			
			
			map<String, String> params;
			if(url.size() > 8) params = extractParams(url.mid(9));
			
			String format = "html";
			if(params.find("format") != params.end()) format = params["format"];
			
			if(format == "" || format == "html") {
				
				socket->writeHttpHeader();
				*socket << "<html><head><title>meteo Server</title>";
				*socket << "<meta http-equiv=\"refresh\" content=\"5\"></head>";
				*socket << "<body>";
				*socket << "<h1>Meteo</h1>\n";
				*socket << "<p><a href=\"index.html\">[Nodes]</a> <a href=\"current?format=html\">[Current]</a></p>\n";
				*socket << "<h2>Current readings</h2>\n";
			
				vector<Station> stations = collector.activeStations();
				*socket << "<table border=\"1\">";
				*socket << "<tr><td><b>Stations</b></td><td><b>Temperature [deg C]</b></td><td><b>Humidity [rel %]</b></td><td><b>Pressure [hPa]</b></td><td><b>Luminositry</b></td></tr>\n";
				for( vector<Station>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
					*socket << "<tr><td><a href=\"node?id=" << (*it).id << "\">" << (*it).name << "</a></td>";
					*socket << "<td>" << round_f((*it).t) << "</td>";
					*socket << "<td>" << round_f((*it).hum) << "</td>";
					*socket << "<td>" << round_f((*it).p) << "</td>";
					*socket << "<td>" << round_f((*it).l_ir) << "/" << round_f((*it).l_vis) << "</td>";
					*socket << "</tr>";
				}
				*socket << "</table>";
				DateTime now;
				*socket << "</p>" << now.format() << "</p>";
			} else if(format == "csv" || format == "plain") {
			
				socket->writeHttpHeader();
				vector<Station> stations = collector.activeStations();
				for( vector<Station>::const_iterator it = stations.begin(); it != stations.end(); ++it) {
					*socket << (*it).id << ',' << (*it).name << ',';
					*socket << round_f((*it).t) << ',';
					*socket << round_f((*it).hum) << ',';
					*socket << round_f((*it).p) << ',';
					*socket << round_f((*it).l_ir) << "/" << round_f((*it).l_vis) << '\n';
				}
			} else {
				socket->writeHttpHeader(400);
				*socket << "Illegal format";
			}
				
		} else if(url.startsWith("/timestamp") || url.startsWith("/timestamp?")) {
			
			map<String, String> params;
			if(url.size() > 10) params = extractParams(url.mid(11));
			
			String format = "html";
			if(params.find("format") != params.end()) format = params["format"];
			if(format == "html" || format == "") {
				socket->writeHttpHeader();
				*socket << "<html><head><title>meteo Server</title>";
				*socket << "<meta http-equiv=\"refresh\" content=\"5\"></head>";
				*socket << "<body>";
				*socket << "<h1>Meteo</h1>\n";
				*socket << "<p><a href=\"index.html\">[Nodes]</a> <a href=\"current?format=html\">[Current]</a></p>\n";
				
				*socket << "<h2>Timestamp</h2>\n";
				DateTime now;
				*socket << now.format() << " (" << now.timestamp() << ")";
			
			} else if(format == "plain" || format == "csv") {
				socket->writeHttpHeader();
				DateTime now;
				*socket << now.timestamp() << ", " << now.format();
				
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
			if(fd <= 0) {
				cerr << "Error accepting http socket: " << strerror(errno) << endl;
				break;
			}
		
			// Socket will be destroyed by thread
			Socket *socket = new Socket(fd);
			// Create new thread for the socket
			pthread_t tid;
			if( pthread_create(&tid, NULL, http_thread_run, socket) < 0) throw "Error creating new http thread";
			pthread_detach(tid);
		}
	
		::close(sock);
	} catch(const char* msg) {
		::close(sock);
		cerr << "Exception in http server thread: " << msg << endl;
		exit(EXIT_FAILURE);
	} catch(...) {
		::close(sock);
		cerr << "Uncaught exception in http server thread" << endl;
		exit(EXIT_FAILURE);
	}
}

static void backtrace() {
	// Print backtrace
	void *array[20];
	size_t size = backtrace(array, sizeof(array) / sizeof(array[0]));
	backtrace_symbols_fd(array, size, STDERR_FILENO);
}

static void terminateHandler() {
	cerr << "Terminating due to uncaught exception" << endl;
	backtrace();
	abort();
}
