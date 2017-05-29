/* =============================================================================
 * 
 * Title:         meteo - Serial Reader and Mosquitto push
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Reads from different serial ports and pushed the data using
 *                the mosquitto protocol
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

#include "server.hpp"
#include "udp.hpp"
#include "string.hpp"
#include "serial.hpp"
#include "node.hpp"
#include "parser.hpp"
#include "database.hpp"


using namespace std;
using flex::String;
using io::Serial;

/** Running flag */
static volatile bool running = true;

/** Data arrival callback */
static void recvCallback(Node &node);

/** Setup serial device for reading. Returns thread id */
static pthread_t  setup_serial(const char* dev);

int main(int argc, const char** argv) {
	vector<const char*> devices;
	for(int i=1;i<argc;i++) {
		string arg(argv[i]);
		if(arg.size() == 0) continue;
		else if(arg.at(0) == '-') {
			if(arg == "-h" || arg == "--help") {
				cout << "meteo - Serial reader and mosquitto push" << endl;
				cout << "Usage: " << argv[0] << "[OPTIONS] DEVICES" << endl;
				cout << "OPTIONS" << endl;
				cout << "  -h  --help             Print this help message" << endl;
				cout << "  -d  --deamon           Start as daemon" << endl;
				
				return EXIT_SUCCESS;
			}
		} else
			devices.push_back(argv[i]);
	}
	
	if(devices.size() == 0) {
		cerr << "No devices defined" << endl;
		cerr << "Type " << argv[0] << " --help if you need help" << endl;
		return EXIT_FAILURE;
	}
	
	vector<pthread_t> threads;
	try {
		for(vector<const char*>::const_iterator it = devices.begin(); it != devices.end(); ++it) {
			pthread_t tid = setup_serial( *it );
			threads.push_back(tid);
		}
	
	} catch (const char* msg) {
		cerr << "Error creating serial threads: " << msg << endl;
		exit(EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	
	cout << threads.size() << " thread(s) started." << endl;
	
	// Wait for threads to terminate
	for(vector<pthread_t>::const_iterator it = threads.begin(); it != threads.end(); ++it) {
		pthread_join(*it, NULL);
	}
	
	cout << "Bye" << endl;
	return EXIT_SUCCESS;
}





/** Thread for setting up serial */
static void* serial_thread(void* arg) {
    const char* dev = (char*)arg;
    
    
	// Thread is cancellable immediately
	//pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    
    cout << "Serial reader for device " << dev << " started" << endl;
    
    int errors = 0;
    while(running) {
    	bool error = false;
		try {
		    Serial serial(dev);
		    Parser parser;
		    while(!serial.eof()) {
		        string line = serial.readLine();
		        if(line.size() == 0 || line.at(0) == '#') continue;
		        
		        // Parse line
				if(parser.parse(line)) {
					Node node = parser.node();
					recvCallback(node);
					parser.clear();
				} else {
					cerr << "Serial read: Illegal input ('" << line << "')" << endl;
				}
		    }
		} catch (const char* msg) {
		    cerr << "Error in serial " << dev << ": " << msg << endl;
		    
		    error = true;
		}
		
		if(error) {
		    errors++;
		    
		    // Sleep to prevent busy waiting
		    int delay = errors*errors;
		    if( (delay < 0) || (delay > 5)) delay = 5;
		    ::sleep(delay);
		} else
			errors = 0;
    }
    
    cout << "Serial thread for " << dev << " terminated" << endl;
    return NULL;
}

static pthread_t setup_serial(const char* dev) {
    // Create thread
    pthread_t tid;
    
	int ret;
	ret = pthread_create(&tid, NULL, serial_thread, (void*)dev);
	if(ret < 0) throw "Error creating serial thread";
	//if(pthread_detach(tid) != 0) throw "Error detaching serial thread";
	return tid;
}

static void recvCallback(Node &node) {
	(void)node;
}
