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

using namespace std;
using flex::String;
using json = nlohmann::json;
using namespace lazy;


/** Mosquitto instances */
static vector<Mosquitto*> mosquittos;
static Collector collector;



/** Mosquitto receive callback */
static void mosq_receive(const std::string &topic, char* buffer, size_t len);
/** Connect a mosquitto server */
static Mosquitto* connectMosquitto(string remote, int port = 1883, string username = "", string password = "");
/** Program cleanup */
static void cleanup();
/** Signal handler */
static void sig_handler(int signo);


int main() { //int argc, char** argv) {
	// Open database
	collector.open_db("meteod.db");
	
    try {
    	connectMosquitto("localhost");
    } catch (const char* msg) {
    	cerr << "Error connecting to mosquitto remote: " << msg << endl;
    	return EXIT_FAILURE;
    }
    
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGUSR1, sig_handler);
    atexit(cleanup);
    collector.start();
    
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
