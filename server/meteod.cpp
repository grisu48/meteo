/* =============================================================================
 * 
 * Title:         meteo Server
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
#include <sys/stat.h>

#include "string.hpp"
#include "config.hpp"
#include "database.hpp"
#include "mosquitto.hpp"
#include "json.hpp"
#include "httpd.hpp"
#include "time.hpp"

using namespace std;
using flex::String;
using json = nlohmann::json;
using namespace lazy;


#define PID_FILE "/var/run/meteod.pid"


static SQLite3Db *db = NULL;
static bool quiet = false;
static bool force = false;
static string pid_file = "";
/** PID for webserver if present */
static pid_t www_pid = 0;

static void received(const int id, const string &name, float t, float hum, float p, float l_vis, float l_ir);

void recv_callback(const std::string &topic, char* buffer, size_t len) {
	(void)len;
	(void)topic;
	
	try {
		string packet(buffer);
		json j = json::parse(packet);
	
		if (j.find("node") == j.end()) return;
		const int id = j["node"].get<int>();		
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
		
		received(id, name, t, hum, p, l_vis, l_ir);
	
	} catch (std::exception &e) {
		cerr << "Exception (" << e.what() << ") parsing packet" << endl << buffer << endl;
	} catch (...) {
		cerr << "Unknown error parsing packet " << buffer << endl;
		return;
	}
}


static void sig_handler(int signo) {
	switch(signo) {
		case SIGINT:
		case SIGTERM:
			exit(EXIT_FAILURE);
			return;
		case SIGCHLD:
			// Webserver terminated
			if(!quiet) cerr << "Error: Webserver terminated" << endl;
			www_pid = 0;
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


static void cleanup() {
	if(db != NULL) delete db;
	db = NULL;
	if(www_pid > 0) {
		kill(www_pid, SIGTERM);
	}
	if(pid_file.size() > 0)
		::remove(pid_file.c_str());
}


bool process_exists(pid_t pid) {
	struct stat sts;
	stringstream ss;
	ss << "/proc/" << pid;
	string filename = ss.str();
	if (stat(filename.c_str(), &sts) == -1 && errno == ENOENT)
		return false;
	else
		return true;
}

bool check_pid_file(const char* filename) {
	if( access( filename, F_OK ) == F_OK ) {
		pid_t pid;
		// Read file, read pid
		ifstream i_pid(filename);
		i_pid >> pid;
		
		if(pid > 0) {
		
			// Check if there is a process attached to it
			if(process_exists(pid)) {
				cout << "PID file exists and process with pid " << pid << " is running" << endl;
				return false;
			} else {
				cout << "PID file exists but no such process is running" << endl;
			}
		
		} else {
			cerr << "PID file " << filename << " exists, but has illegal contents - Remove file" << endl;
			if(force) 
				::remove(filename);
			return false;
		}
	}
	
	// Write pid
	pid_t pid = getpid();
	ofstream o_pid(filename);
	if(o_pid.is_open()) 
		o_pid << pid;
	else
		cerr << "Error writing pid to file " << filename << endl;
	return true;
}

int main(int argc, char** argv) {
	string remote = "";
	string topic = "meteo/#";
	string db_filename = "meteo.db";
	vector<string> topics;
	bool daemon = false;
	bool verbose = false;
	int www = 0;			// if > 0 a webserver will be forked on that port
	
	// XXX: Replace with getopt
	for(int i=1;i<argc;i++) {
		string arg(argv[i]);
		if(arg.size() == 0) continue;
		if(arg.at(0) == '-') {
			if(arg == "-h" || arg == "--help") {
				cout << "Meteo Server instance" << endl;
				cout << "  2017 - Felix Niederwanger" << endl;
				cout << "Usage: " << argv[0] << " [OPTIONS] REMOTE" << endl;
				cout << "OPTIONS" << endl;
				cout << "   -h    --help             Print this help message" << endl;
				cout << "   -d    --daemon           Run as daemon" << endl;
				cout << "   -q    --quiet            Quiet run" << endl;
				cout << "   -f    --force            Force run" << endl;
				cout << "   -v    --verbose          Verbose run (overrides quiet)" << endl;
				cout << "   -t TOPIC  --topic TOPIC  Set TOPIC to subscribe from server" << endl;
				cout << "                            If no topic is set, the program will use 'meteo/#' as default" << endl;
				cout << "         --http PORT        Setup webserver on PORT" << endl;
				cout << "         --db FILE          Set file for database" << endl;
				cout << "   --pid-file FILE          Set PID-file (Default: " << PID_FILE << " when daemon)" << endl;
				cout << endl;
				cout << "REMOTE is the mosquitto remote end" << endl;
				return EXIT_SUCCESS;
			} else if(arg == "-d" || arg == "--daemon") 
				daemon = true;
			else if(arg == "-q" || arg == "--quiet")
				quiet = true;
			else if(arg == "-v" || arg == "--verbose")
				verbose = true;
			else if(arg == "-f" || arg == "--force")
				force = true;
			else if(arg == "-t" || arg == "--topic") {
				// XXX : Check if last argument
				topics.push_back(string(argv[++i]));
			} else if(arg == "--http") {
				// XXX : Check if last argument
				www = ::atoi(argv[++i]);
			} else if(arg == "--db") {
				// XXX : Check if last argument
				db_filename = argv[++i];
			} else if(arg == "--pid-file") {
				// XXX : Check if last argument
				pid_file = argv[++i];
			} else {
				cerr << "Illegal argument: " << arg << endl;
				return EXIT_FAILURE;
			}
			
		} else
			remote = arg;
	}
	if(verbose) quiet = false;		// Verbose overrides quiet

	if(remote.size() == 0) {
		cerr << "No mosquitto remote set" << endl;
		cerr << "  Usage: " << argv[0] << " [OPTIONS] REMOTE" << endl;
		cerr << "  REMOTE defines a mosquitto server" << endl;
		cerr << "  Type " << argv[0] << " --help if you need help" << endl;
		return EXIT_FAILURE;
	}
	
	// Check for PID file
	if (daemon && pid_file == "") pid_file = PID_FILE;
	if(pid_file.size() > 0) {
		if (pid_file == "") pid_file = PID_FILE;
		if(!check_pid_file(pid_file.c_str()) && !force)
			return EXIT_FAILURE;
	}
	
	// At this point fork webserver
	if(www > 0) {
		if(verbose) cout << "Starting webserver on port " << www << " ... " << endl;
		pthread_t tid = Webserver::startThreaded(www, db_filename);
		if(verbose) cout << "Webserver running as thread " << tid << endl;
	}

	db = new SQLite3Db(db_filename);
	db->exec_noResultSet("CREATE TABLE IF NOT EXISTS `Nodes` (`id` INT PRIMARY KEY, `name` TEXT);");

	Mosquitto mosq(remote);	
	try {
		if(verbose) cout << "Connecting to " << remote << " ... " << endl;
		mosq.connect();
		mosq.setReceiveCallback(recv_callback);
		// Subscribe to topics or "meteo/#" if no topics are defined
		if(topics.size() == 0) {
			mosq.subscribe("meteo/#");
			if(verbose) cout << "  Subscribing to 'meteo/#' as default topic" << endl;
		} else {
			for(vector<string>::const_iterator it = topics.begin(); it != topics.end(); ++it) {
				mosq.subscribe(*it);
				if(verbose) cout << "  Subscribing to '" << (*it) << "'" << endl;
			}
		}
	} catch (const char *msg) {
		cerr << "Error setting up mosquitto: " << msg << endl;
		return EXIT_FAILURE;
	}
	
	if(daemon) fork_daemon();
	atexit(cleanup);
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	
	try {
		if(verbose) cout << "Mosquitto looper ... " << endl;
		mosq.loop();
	} catch (const char* msg) {
		cerr << "Error looping in mosquitto: " << msg << endl;
		return EXIT_FAILURE;
	}
	
	cout << "Done" << endl;
	return 0;
}

static bool hasNode(const int id) {
	if(db == NULL) return false;
	
	stringstream ss;
	ss << "SELECT * FROM `Nodes` WHERE `id` = " << id << ";";
	SQLite3ResultSet *rs = db->doQuery(ss.str());
	bool result = rs->next();
	rs->close();
	return result;
}

static void received(const int id, const string &name, float t, float hum, float p, float l_vis, float l_ir) {
	(void)l_vis;
	(void)l_ir;
	
	
	if(!quiet) cout << id << " [" << name << "], t=" << t << " deg C, " << hum << " % rel. humidity, " << p << " hPa" << endl;
	if(db == NULL) return;
	
	try {
	
		stringstream ss;
	
		// Check if node is in node list
		if(!hasNode(id)) {
			ss << "INSERT OR REPLACE INTO `Nodes` (`id`, `name`) VALUES (" << id << ", '" << db->escapeString(name) << "');";
			db->exec_noResultSet(ss.str());
		}
	
		// Create table if not exists	
		ss << "CREATE TABLE IF NOT EXISTS `Node_" << id << "` (`timestamp` INT PRIMARY KEY, `t` REAL, `hum` REAL, `p` REAL, `l_vis` REAL, `l_ir` REAL);";
		db->exec_noResultSet(ss.str());
		ss.str("");
	
		const int64_t timestamp = getSystemTime();
	
		// Insert values into database
		ss << "INSERT OR REPLACE INTO `Node_" << id << "` (`timestamp`, `t`, `hum`, `p`, `l_vis`, `l_ir`) VALUES (";
		ss << timestamp << ", " << t << ", " << hum << ", " << p << ", " << l_vis << ", " << l_ir << ");";
		db->exec_noResultSet(ss.str());
	} catch(...) {
		throw;
	}
}
