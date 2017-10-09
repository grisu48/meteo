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
#include <sys/types.h>
#include <sys/stat.h>

#include "serial.hpp"

using namespace std;

struct mosquitto *mosq = NULL;



static void fork_daemon(void);

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
		buf << "1 every " << seconds << " s";
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
		cerr << " (Rate: " << getRate(avg) << ')';
	}
	cerr << endl;
	
	// Publish but not more than 1x per second
	if(delta > 1000L) {
		// TODO: Implement me
	}
}

static void noise_detected(const long millis) {
	static long last_millis = millis;
	long delta = millis - last_millis;
	last_millis = millis;
	
	cerr << '[' << getTime() << "] " << millis << " Noise detected";
	if(delta > 0)
		cerr << " (Rate: " << getRate(delta) << ')';
	cerr << endl;
	
	// Publish but not more than 1x per second
	if(delta > 1000L) {
		// TODO: Implement me
	}
}


static bool is_block(const char* device) {
	struct stat s;
	stat(device, &s );
	return S_ISBLK(s.st_mode);
}

int main(int argc, char** argv) {
    string device = "-";							// File or serial device
    const char* mqtt_host = "";		            // Mosquitto server
    int mqtt_port = 1883;						// Mosquitto server port
    int node_id = 1;							// This node id
    int rc;										// Return codes
    string topic = "";							// Topic for publishing
	speed_t baud = B9600;						// Baud rate
	bool daemonize = false;
    
    try {
		for(int i=1;i<argc;i++) {
			string arg(argv[i]);
			if(arg.size() == 0) continue;
			if(arg.at(0) == '-') {
				if(arg == "-h" || arg == "--help") {
					cout << "Lightning daemon | Reads serial output from a AS3935 Arduino and publish data on mosquitto" << endl;
					cout << "  2017, Felix Niederwanger" << endl << endl;
					cout << "SYNPOSIS: " << argv[0] << " DEVICE/FILE [OPTIONS]" << endl;
					cout << "  DEVICE/FILE     Serial device or input file (Default: " << device << ") - Use '-' for stdin" << endl;
					cout << "OPTIONS:" << endl;
					cout << "   -h      --help          Print help message" << endl;
					cout << "   -t TOPIC                Manually set topic for mosquitto publish" << endl;
					cout << "   -h HOST                 Mosquitto server hostname (Default: Disabled)" << endl;
					cout << "   -p PORT                 Mosquitto server port (Default: " << mqtt_port << ")" << endl;
					cout << "   -d      --daemon        Run as daemon" << endl;
					return EXIT_SUCCESS;
				} else if(arg == "-t" || arg == "--topic") {
					topic = argv[++i];
				} else if(arg == "-h" || arg == "--host" || arg == "--mosquitto") {
					mqtt_host = argv[++i];
				} else if(arg == "-p" || arg == "--port") {
					mqtt_port = ::atoi(argv[++i]);
				} else if(arg == "-d" || arg == "--daemon") {
					daemonize = true;
				} else {
					cerr << "Illegal argument: " << arg << endl;
					exit(EXIT_FAILURE);
				}
			} else
				device = argv[i];
		}
    } catch (const char* msg) {
    	cerr << msg << endl;
    	exit(EXIT_FAILURE);
    }
    
    // Build topic
    if(topic.size() == 0) {
    	stringstream buf;
    	buf << "lightning/" << node_id;
    	topic = buf.str();
    }
    
    if(daemonize)
    	fork_daemon();
    
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
   	Serial *serial = NULL;
    try {
    	bool isBlockDevice = (device != "" && device != "-") && is_block(device.c_str());
    	if(isBlockDevice) {
    		serial = new Serial(device.c_str(),false);
    		serial->setSpeed(baud);
	    	if(serial->isNonBlocking())
    			cerr << "WARNING: Device is nonblocking" << endl;
    	}
    	
    	
    	while(true) {
    		string line;
    		
    		if(serial != NULL) {
    			if(serial->eof()) break;
    			line = serial->readLine();
    		} else {
    			if(device == "" || device == "-") {
    				if(cin.eof()) break;
    				getline(cin, line);
    			} else {
    				throw "File read not yet supported";
    			}
    		}
    		line = trim(line);
    		if(line.size() == 0) continue;
    		
    		if(line.at(0) == '#') {		// Comment from device
    			cerr << line << endl;
    			continue;
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
	    			} else if (s_message.substr(0,9) == "DETECTED ") {
	    				try {
	    					string s_distance = s_message.substr(9, s_message.size()-9-3 );
	    					float distance = (float)::atof(s_distance.c_str());
	    					
	    					
							if(mosq != NULL || true) {
	    						// Build package
								stringstream buf;
								long timestamp = millis;
								buf << "{\"station\":" << node_id << ",\"timestamp\":" << timestamp << ",\"distance\":" << distance << "}";
								string msg = buf.str();
								
				    			publish(topic, msg);
				    		}
				    		
				    		cout << '[' << getTime() << "] " << millis << "\tDetected a lightning in " << distance << "km" << endl;
	    					
	    				} catch(const char* msg) {
	    					cerr << "Illegal packet read: " << msg << endl;
	    				}
	    				
						//cout << '[' << getTime() << "] " << millis << '\t' << s_message << endl;
					
	    			} else if (s_message.substr(0,18) == "UNKNOWN INTERRUPT ") {
	    				string s_interrupt = s_message.substr(18);
	    				s_interrupt = trim(s_interrupt);
	    				cerr << '[' << getTime() << "] " << millis << "\tUnknown interrupt: " << s_interrupt << endl;
	    			} else {
	    				
						cerr << '[' << getTime() << "] Illegal packet: " << line << endl;
	    				
	    			}
	    		}
	    	}
    		
    	}
    	// Should never occur
    	throw "EOF reached";
    	
    } catch(const char* msg) {
    	cerr << "Serial port error: " << msg << endl;
	    if(serial != NULL) delete serial;
    	exit(EXIT_FAILURE);
    }
    
    if(serial != NULL) delete serial;
    cout << "Bye" << endl;
    return EXIT_SUCCESS;
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
