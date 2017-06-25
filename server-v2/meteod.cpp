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

#include "string.hpp"
#include "config.hpp"
#include "database.hpp"
#include "mosquitto.hpp"
#include "json.hpp"
#include "time.hpp"

using namespace std;
using flex::String;
using json = nlohmann::json;
using namespace lazy;

static SQLite3Db *db = NULL;
static bool quiet = false;


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
}

int main(int argc, char** argv) {
	string remote = "";
	string db_filename = "meteo.db";
	bool daemon = false;
	
	
	for(int i=1;i<argc;i++) {
		string arg(argv[i]);
		if(arg.size() == 0) continue;
		if(arg.at(0) == '-') {
			if(arg == "-h" || arg == "--help") {
				cout << "Meteo Server instance" << endl;
				cout << "  2017 - Felix Niederwanger" << endl;
				cout << "Usage: " << argv[0] << " [OPTIONS]" << endl;
				cout << "OPTIONS" << endl;
				cout << "   -h    --help             Print this help message" << endl;
				cout << "   -d    --daemon           Run as daemon" << endl;
				cout << "   -q    --quiet            Quiet run" << endl;
			} else if(arg == "-d" || arg == "--daemon") 
				daemon = true;
			else if(arg == "-q" || arg == "--quiet")
				quiet = true;
			else {
				cerr << "Illegal argument: " << arg << endl;
				return EXIT_FAILURE;
			}
			
		} else
			remote = arg;
	}

	if(remote.size() == 0) {
		cerr << "No remote set" << endl;
		return EXIT_FAILURE;
	}

	db = new SQLite3Db(db_filename);
	db->exec_noResultSet("CREATE TABLE IF NOT EXISTS `Nodes` (`id` INT PRIMARY KEY, `name` TEXT);");

	Mosquitto mosq(remote);	
	try {
		mosq.connect();
		mosq.setReceiveCallback(recv_callback);
		mosq.subscribe("meteo/#");
	} catch (const char *msg) {
		cerr << "Error setting up mosquitto: " << msg << endl;
		return EXIT_FAILURE;
	}
	
	if(daemon) fork_daemon();
	atexit(cleanup);
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	
	try {
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
}
