/* =============================================================================
 * 
 * Title:         Meteo2 Main program
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Meteo2 sensor node
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


using namespace std;
using namespace sensors;
using namespace meteo;
using flex::String;



// Sensor enable switches (1 = ENABLED, 0 = DISABLED)
#define SENSOR_BMP180 1
#define SENSOR_HTU21DF 1
#define SENSOR_MCP9808 1
#define SENSOR_TSL2561 1

#define DEFAULT_UDP_PORT 5232

/* ==== CLASS DECLARATIONS ================================================== */


/** Mosquitto helper class */
class Mosquitto {
private:

public:
	
};






int main(int argc, char** argv) {
	// Parse program arguments
	try {
		for(int i=1;i<argc;i++) {
			string arg(argv[i]);
			if(arg == "--help") {
				return EXIT_SUCCESS;
			} else {
			
			}
		}
	} catch (const char* msg) {
		cerr << "Error parsing arguments: " << msg << endl;
		return EXIT_FAILURE;
	}
	
	
	// Done
	return EXIT_SUCCESS;
}


static int readSensor(Sensor &sensor) {
	int tries = 5;
	int ret;
	while(tries-- > 0) {
		ret = sensor.read();
		if(ret == 0) return ret;		// Done
		
		// Could be that someone else is currently reading, therefore we
		// wait a little bit and try again
		p_sleep(100);
	}
	return ret;
}
