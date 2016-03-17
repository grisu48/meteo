/* =============================================================================
 * 
 * Title:         HESSberry Monitor Shared memory reader file
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */

#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/shm.h>
#include <cstdlib>
#include <errno.h>
#include <string.h>
#include <errno.h>

#include "monitor.hpp"



using namespace std;


static shm_t *memory = NULL;


static void cleanup(void) {
	// Detach
	if(memory != NULL) {
		shmdt(memory);
	}
}

static void printShm(void) {
	// Read shm
	shm_t local;
	memcpy(&local, memory, sizeof(shm_t));
	
	if (local.state == STATE_SETUP)
		cout << "Status:   Setup" << endl;
	else if (local.state == STATE_READ)
		cout << "Status:   Reading" << endl;
	else if (local.state == STATE_CLOSED)
		cout << "Status:   Closed" << endl;
	else if (local.state == STATE_ERROR)
		cout << "Status:   Error" << endl;
	else if (local.state == STATE_OK)
		cout << "Status:   Ok" << endl;
	
	cout << "Timestamp:          " << local.timestamp << "" << endl;
	cout << "Temperature:        " << local.temperature << " deg C" << endl;
	cout << "Humidity:           " << local.humidity << " % rel" << endl;
	cout << "Lux (Broadband):    " << local.lux_broadband << "" << endl;
	cout << "Lux (Visible):      " << local.lux_visible << "" << endl;
	cout << "Lux (Infrared):     " << local.lux_ir << "" << endl;
	
}


int main(int argc, char** argv) {
	int shm_id = 7443;
	int delay = 0;			// Delay or 0 if single read
	
	for(int i=1;i<argc;i++) {
		string arg = string(argv[i]);
		const bool isLast = i >= argc-1;
		
		try {
			if(arg == "-h" || arg == "--help") {
				cout << "Meteo - HESSberry Monitor Shared memory reader" << endl;
				cout << "  2016 Felix Niederwanger <felix.niederwanger@uibk.ac.at>" << endl;
				cout << "Usage: " << argv[0] << " [OPTIONS]" << endl;
				cout << "  OPTIONS: " << endl;
				
				cout << "  -d N  --delay N          Continuous read each N seconds" << endl;
				cout << "  --id ID                  Set shared memory id to ID (Default: 7443)" << endl;
				
				return EXIT_SUCCESS;
			} else if(arg == "-d" || arg == "--delay") {
				if(isLast) throw "Missing argument: Delay";
				delay = atoi(argv[++i]);
				if(delay < 0) delay = 0;
			} else if(arg == "--id") {
				if(isLast) throw "Missing argument: shm id";
				shm_id = atoi(argv[++i]);
				if(shm_id <= 0) throw "Illegal shm id";
			} else
				throw "Illegal parameter";
		} catch (const char* msg) {
			cerr << "Error parsing argument " << arg << ": " << msg << endl;
			return EXIT_FAILURE;
		}
	}
	
	atexit(cleanup);
	
	key_t id = shm_id;
	shm_id = shmget(id, sizeof(shm_t), 0444);
	if(shm_id < 0) {
		cerr << "Error " << errno << " getting shared memory: " << strerror(errno) << endl;
		return EXIT_FAILURE;
	}
	
	// Attach
	memory = (shm_t*)shmat(shm_id, 0,0);
	if(memory == NULL) {
		cerr << "Error attaching memory: " << strerror(errno) << endl;
		return EXIT_FAILURE;
	}
	
	printShm();
	
	// Contiuous read?
	if(delay > 0) {
		while(true) {
			sleep(delay);
			printShm();
		}
	}

	return EXIT_SUCCESS;	
}

