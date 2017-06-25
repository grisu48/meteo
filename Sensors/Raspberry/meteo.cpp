/* =============================================================================
 * 
 * Title:         Meteo Main program
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Meteo sensor node
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>


#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

// Include sensors
#include "sensors.hpp"
#include "config.hpp"
#include "string.hpp"
#include "mosquitto.hpp"

using namespace std;
using namespace sensors;
using namespace meteo;
using flex::String;


#define CONFIG_FILE "meteo.cf"


static Mosquitto *mosq = NULL;
vector<Sensor*> _sensors;
static bool running = true;





static int p_sleep(long millis, long micros = 0) {
       struct timespec tt, rem;
       int ret;

       tt.tv_sec = millis/1000L;
       millis -= tt.tv_sec*1000L;
       tt.tv_nsec = (micros + millis*1000L)*1000L;
       ret = nanosleep(&tt, &rem);
       if(ret < 0) {
               switch(errno) {
               case EFAULT:
                       return -1;
               case EINTR:
                       // Interrupted
                       if(!running) return -2;
                       
                       // Is still running, so sleep remaining time
                       millis = rem.tv_sec * 1000L;
                       micros = rem.tv_nsec / 1000L;
                       millis += micros/1000L;
                       micros -= millis * 1000L;
                       return p_sleep(millis, micros);
               case EINVAL:
                       return -3;
               default :
                       return -8;
               }
       } else 
               return 0;
}


static int readSensor(Sensor *sensor) {
	int tries = 5;
	int ret;
	while(tries-- > 0) {
		ret = sensor->read();
		if(ret == 0) return ret;		// Done
		
		// Could be that someone else is currently reading, therefore we
		// wait a little bit and try again
		p_sleep(100);
	}
	return ret;
}

static void cleanup() {
	// Delete sensors
	vector<Sensor*> sensors(_sensors);
	_sensors.clear();
	for(vector<Sensor*>::iterator it = sensors.begin(); it != sensors.end(); ++it)
		delete *it;
}

static void sig_handler(int signo) {
	switch(signo) {
		case SIGINT:
		case SIGTERM:
			if(!running) exit(EXIT_FAILURE);
			running = false;
			break;
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

int main(int argc, char** argv) {
	string i2c = "/dev/i2c-2";
	string mosquitto = "";
	string name = "";			// Node name, if available
	// Sensor enable flags
	bool bmp180 = false;
	bool htu21df = false;
	bool mcp9808 = false;
	bool tsl2561 = false;
	bool daemon = false;
	bool quiet = false;			// Quiet mode
	int delay = 5;				// Delay between pushes
	int node_id = 0;			// ID of the node
	
	// Read config
	{
		string tmp;
		Config config(CONFIG_FILE);
		if(!config.readSuccessfull()) {
			cerr << "Reading of the config file failed." << endl;
			cerr << "Please check your config file " << CONFIG_FILE << endl;
			return EXIT_FAILURE;
		}
		if((tmp = config.get("mosquitto", "")) != "")
			mosquitto = tmp;
		if((tmp = config.get("i2c", "")) != "")
			i2c = tmp;
		bmp180 = config.getBoolean("bmp180", bmp180);
		htu21df = config.getBoolean("htu21df", htu21df);
		mcp9808 = config.getBoolean("mcp9808", mcp9808);
		tsl2561 = config.getBoolean("tsl2561", tsl2561);
		node_id = config.getInt("id", node_id);
		quiet = config.getBoolean("quiet", quiet);
		daemon = config.getBoolean("daemon", daemon);
		delay = config.getInt("delay", delay);
		name = config.get("name", "");
	}
	
	for(int i=1;i<argc;i++) {
		string arg(argv[i]);
		if(arg == "-h" || arg == "--help") {
			cout << "Meteo Sensor - v2.0" << endl;
			cout << "  2017 Felix Niederwanger" << endl;
			
			cout << "Usage: " << argv[0] << " [OPTIONS]" << endl;
			cout << "OPTIONS:" << endl;
			cout << "    -h     --help               Print this help message" << endl;
		} else if(arg == "--all") {
			bmp180 = true;
			htu21df = true;
			mcp9808 = true;
			tsl2561 = true;
		} else if(arg == "--id") {
			node_id = ::atoi(argv[++i]);		// XXX: Potentially index-out-of-bands!
		} else if(arg == "--quiet" || arg == "-q") {
			quiet = true;
		} else if(arg == "--daemon" || arg == "-d") {
			daemon = true;
		} else if(arg == "--delay") {
			delay = ::atoi(argv[++i]);		// XXX: Potentially index-out-of-bands!
			if(delay <= 0) delay = 1;
		} else {
			cerr << "Illegal argument: " << arg << endl;
			return EXIT_FAILURE;
		}
	}
	
	if(mosquitto == "") {
		cerr << "WARNING: No mosquitto server defined. No data will be published!" << endl;
	} else {
		mosq = new Mosquitto();
		mosq->connect(mosquitto.c_str());
	}
	
	// Setting up sensors
	if(bmp180)
		_sensors.push_back(new BMP180(i2c.c_str()));
	if(htu21df)
		_sensors.push_back(new HTU21DF(i2c.c_str()));
	if(mcp9808)
		_sensors.push_back(new MCP9808(i2c.c_str()));
	if(tsl2561)
		_sensors.push_back(new TSL2561(i2c.c_str()));
	
	if(_sensors.size() == 0) {
		cerr << "Error: No sensors set" << endl;
		cerr << "Type " << argv[0] << " --help if you need help. " << argv[0] << " --all enables all sensors" << endl;
		return EXIT_FAILURE;
	}
	
	if(daemon) fork_daemon();
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);
	atexit(cleanup);
	
	while(running) {
		// Read sensors
		bool first = true;
		for(vector<Sensor*>::const_iterator it = _sensors.begin(); it != _sensors.end(); ++it) {
			readSensor(*it);
			if(!quiet) {
				if(first) first = false;
				else cout << ", ";
				cout << (*it)->toString();
			}
		}
		if(!quiet) cout << endl;
		
		if(mosq != NULL) {
			// Build json packet
			stringstream ss;
			
			ss << "{\"node\":" << node_id;
			if(name.size() > 0)
				ss << ",\"name\":\"" << name << "\"";
			
			for(vector<Sensor*>::const_iterator it = _sensors.begin(); it != _sensors.end(); ++it) {
				map<string,float> values = (*it)->values();
				
				for(map<string,float>::const_iterator j = values.begin(); j != values.end(); j++) {
					ss << ",\"" << j->first << "\":" << j->second;
				}
			}
			
			ss << "}";
			
			string json = ss.str();
			ss.str("");
			ss << "meteo/" << node_id;
			string topic = ss.str();
			
			mosq->publish(topic, json);
			if(!quiet) cout << topic << " :: " << json << endl;
		}
		
		sleep(delay);
	}
	
	
	return 0;
}
