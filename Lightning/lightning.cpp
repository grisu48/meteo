/* =============================================================================
 * 
 * Title:         
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <string>

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <mosquitto.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <math.h>

#include "serial.hpp"

using namespace std;

struct mosquitto *mosq = NULL;



// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}


void cleanup() {
	if(mosq != NULL) {
		mosquitto_loop_stop(mosq, true);
		mosquitto_destroy(mosq);
		mosq = NULL;
	}
	mosquitto_lib_cleanup();
}

/** Publish the given message to the given topic */
int publish(const string &topic, const string &payload) {
    if(mosq == NULL) return 0;
    
	size_t len = payload.size();
	const void* buf = (void*)payload.c_str();
	
	int tries = 3;
	int rc = 0;
	while(tries-- > 0) {
		rc = mosquitto_publish(mosq, 0, topic.c_str(), len, buf, 0, true);
		if(rc == MOSQ_ERR_SUCCESS) return rc;
		else {
			if(mosquitto_reconnect(mosq) != MOSQ_ERR_SUCCESS) {
				cerr << "Mosquitto reconnect failed: " << strerror(errno) << endl;
				continue;
			}
		}
	}
	return rc;
}

string getTime(const char* format = "%Y-%m-%d %H:%M:%S") {
	char stime[128];
	time_t rawtime;
	struct tm *info;
   
	time( &rawtime );
	info = localtime( &rawtime );
	
	memset(stime, '\0', 128);
	strftime(stime,128, format, info);
	stime[127] = '\0';
	return string(stime);
}

static string getRate(const long delta) {
	if (delta == 0) return "";
	
	stringstream buf;
	
	float rate = round(1000.0F / delta * 1e2) / 1e2;
	if(rate <= 0.0F) {
		const long seconds = delta/1000L;
		buf << "each " << seconds << " s";
	} else
		buf << rate << "/s";
	
	return buf.str();
}

static void disturber_detected(const long millis) {
	static long last_millis = millis;
	long delta = millis - last_millis;
	last_millis = millis;
	
	cerr << '[' << getTime() << "] " << millis << " Disturber detected";
	if(delta > 0) {
		static long avg = delta;
		avg = (0.75*avg) + 0.25*delta;
		cerr << " (Rate of ~ " << getRate(avg) << ')';
	}
	cerr << endl;
}

static void noise_detected(const long millis) {
	static long last_millis = millis;
	long delta = millis - last_millis;
	last_millis = millis;
	
	cerr << '[' << getTime() << "] " << millis << " Noise detected";
	if(delta > 0)
		cerr << " (" << getRate(delta) << ')';
	cerr << endl;
}

int main(int argc, char** argv) {
    const char* device = "/dev/ttyUSB0";		// Serial port
    const char* mqtt_host = "";		            // Mosquitto server
    int mqtt_port = 1883;						// Mosquitto server port
    int rc;		// Return codes
    string topic = "lightning/1";				// Topic for publishing
    
    for(int i=1;i<argc;i++) {
    	string arg(argv[i]);
    	if(arg.size() == 0) continue;
    	if(arg == "-h" || arg == "--help") {
    		cout << "Lightning daemon" << endl << "  2017, phoenix" << endl << endl;
    		cout << "SYNPOSIS: " << argv[0] << " DEVICE [HOST] [TOPIC] [PORT]" << endl;
    		cout << "  DEVICE      Serial device (Default: " << device << ")" << endl;
    		cout << "  HOST        Mosquitto server hostname (Default: " << mqtt_host << ")" << endl;
    		cout << "  PORT        Mosquitto server port (Default: " << mqtt_port << ")" << endl;
    		return EXIT_SUCCESS;
    	}
    }
    if(argc > 1) 
    	device = argv[1];
    if(argc > 2)
    	mqtt_host = argv[2];
    if(argc > 3) 
    	mqtt_port = ::atoi(argv[3]);
    
    atexit(cleanup);
    
    // Setting up mosquitto
    if(strlen(mqtt_host) > 0) {
        mosquitto_lib_init();
        mosq = mosquitto_new(NULL, true, NULL);
        if(!mosq) {
    	    cerr << "Error setting up mosquitto instance" << endl;
    	    return EXIT_FAILURE;
        }
    
        rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);
        if(rc != MOSQ_ERR_SUCCESS) {
        	cerr << "Error connecting to mosquitto host " << mqtt_host << ":" << mqtt_port << endl;
        	return EXIT_FAILURE;
        }
        
        // starting mosquitto loop
        rc = mosquitto_loop_start(mosq);
        if(rc != MOSQ_ERR_SUCCESS) {
        	cerr << "Error starting mosquitto looper thread" << endl;
        	exit(EXIT_FAILURE);
        	return EXIT_FAILURE;
        }
    
    }
    
    // Open device
    try {
    	Serial serial(device, false);
    	
    	if(serial.isNonBlocking())
    		cerr << "WARNING: Device is nonblocking" << endl;
    	
    	while(!serial.eof()) {
    		string line = serial.readLine();
    		line = trim(line);
    		if(line.size() == 0) continue;
    		
    		if(line.at(0) == '#') {
    			cerr << line << endl;
    			continue;		// Comment from device
    		} else {
	    		// Parse line
	    		size_t i = line.find(' ');
	    		if(i == string::npos) {
	    			cerr << "Syntax error: " << line << endl;
	    			continue;
	    		} else {
	    			if(i>=line.size()) continue;
	    			
	    			string s_millis = line.substr(0,i);
	    			string s_message = line.substr(i+1);
	    			s_message = trim(s_message);
	    			
	    			long millis = ::atol(s_millis.c_str());
	    			
	    			// Check message
	    			if(s_message == "NOISE DETECTED") {
	    				noise_detected(millis);
	    			} else if(s_message == "DISTURBER DETECTED") {
	    				disturber_detected(millis);
	    			} else {
	    				// Assume it is lightning detected
	    				// XXX: Maybe include a check?
	    			
						// Publish
						stringstream buf;
						buf << '[' << getTime() << "]\t" << s_message;
						string msg = buf.str();
						
						if(mosq != NULL)
			    			publish(topic, msg);
						
						cout << '[' << getTime() << "] " << millis << '\t' << s_message << endl;
	    			}
	    		}
	    	}
    		
    	}
    	// Should never occur
    	throw "EOF reached";
    	
    } catch(const char* msg) {
    	cerr << "Serial port error: " << msg << endl;
    	exit(EXIT_FAILURE);
    	return EXIT_FAILURE;
    }
    
    cout << "Bye" << endl;
    return EXIT_SUCCESS;
}
