/* =============================================================================
 * 
 * Title:         HESSberry monitor
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <sstream>

#include <cstdlib>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>


#include "TSL2561.c"
#include "htu21dflib.c"

#include "udp.cpp"
#include "database.hpp"
#include "monitor.hpp"
#include "server.cpp"

using namespace std;
using namespace hess;







/* ==== Static program variables ============================================ */
// Yes, I know they are ugly, but this code was written really fast and in a 
//   haste.



static bool running = true;				// Running flag
FILE *file = NULL;						// File object for writing the logs
Database *database = NULL;				// Database instance
static long db_log_delay = 120;			// Delay in seconds between database loggings
static float ALPHA = 0.9;				// Smoothing factor
static pthread_t db_thread;				// Database thread
static pthread_t srv_thread;			// Server thread
static int srv_port = 0;				// Server port or 0, if disabled
static Server server;					// Server instance
static shm_t *shm_memory;				// Shared memory structure
static string name = "meteo";			// Name of this station
static int shm_id = 0;					// Shared memory ID, if desired (0 = disabled)
static volatile bool nocleanup = false;	// Flag to delay the cleanup (while writing)

static const char* I2CDEV = "/dev/i2c-1";  			// i2c bus device name
static const uint8_t I2C_ADDR_HTU21DF = 0x40;		// htu21df i2c address




/* ==== Program functions and prototypes ==================================== */


/* Signal handler */
static void sig_handler(int sig) {
	switch (sig) {
		case SIGINT:
		case SIGTERM:
			if(!running) {
				cerr << "Terminating" << endl;
				exit(EXIT_FAILURE);
			}
			cerr << "Interrupt signal. Gracefully shut down ..." << endl;
			cerr << "  Send signal again to terminate program" << endl;
			running = false;
	}
}

static void cleanup(void) {
	while(nocleanup);			// Wait for nocleanup done
	
	if(file!=NULL) {
		fclose(file);
		file = NULL;
	}
	if(database != NULL) 
		delete database;
	database = NULL;
	
	if(shm_id > 0) {
		shm_memory->state = STATE_CLOSED;		// Exit
		shmdt(shm_memory);
	}
}






long long getSystemTime(void) {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    return milliseconds;
}

static int read_htu21df(htu21df_t *result) {
    int rc;     // return code
    int i2cfd;  // i2c file descriptor
    float temperature, humidity;
    //char curlstr[256];

    i2cfd = i2c_open(I2CDEV);
    if (i2cfd < 0) {
//        printf("i2c_open failed %d\n", i2cfd);
        return -1;
    }

    rc = htu21df_init(i2cfd, I2C_ADDR_HTU21DF);
    if (rc < 0) {
//        printf("i2c_init failed %d\n", rc);
        return -2;
    }

    rc = htu21df_read_temperature(i2cfd, &temperature);
    if (rc < 0) {
//            printf("i2c_read_temperature failed %d\n", rc);
        return -3;
    }

    rc = htu21df_read_humidity(i2cfd, &humidity);
    if (rc < 0) {
//            printf("i2c_read_humidity failed %d\n", rc);
        return -4;
    }

    rc = i2c_close(i2cfd);
    if (rc < 0) {
//        printf("i2c_close failed %d\n", rc);
        return -5;
    }
    
    result->temperature = temperature;
    result->humidity = humidity;
    
    return 0;
}

static int read_tsl2561(tsl2561_t *tsl2561) {
	int rc;
	uint16_t broadband, ir;
	uint32_t lux=0;
	// prepare the sensor
	// (the first parameter is the raspberry pi i2c master controller attached to the TSL2561, the second is the i2c selection jumper)
	// The i2c selection address can be one of: TSL2561_ADDR_LOW, TSL2561_ADDR_FLOAT or TSL2561_ADDR_HIGH
	TSL2561 light1 = TSL2561_INIT(1, TSL2561_ADDR_FLOAT);
	
	// initialize the sensor
	rc = TSL2561_OPEN(&light1);
	if(rc != 0) {
//		fprintf(stderr, "Error initializing TSL2561 sensor (%s). Check your i2c bus (es. i2cdetect)\n", strerror(light1.lasterr));
		// you don't need to TSL2561_CLOSE() if TSL2561_OPEN() failed, but it's safe doing it.
		TSL2561_CLOSE(&light1);
		return -1;
	}
	
	// set the gain to 1X (it can be TSL2561_GAIN_1X or TSL2561_GAIN_16X)
	// use 16X gain to get more precision in dark ambients, or enable auto gain below
	rc = TSL2561_SETGAIN(&light1, TSL2561_GAIN_1X);
	
	// set the integration time 
	// (TSL2561_INTEGRATIONTIME_402MS or TSL2561_INTEGRATIONTIME_101MS or TSL2561_INTEGRATIONTIME_13MS)
	// TSL2561_INTEGRATIONTIME_402MS is slower but more precise, TSL2561_INTEGRATIONTIME_13MS is very fast but not so precise
	rc = TSL2561_SETINTEGRATIONTIME(&light1, TSL2561_INTEGRATIONTIME_101MS);
	
	// sense the luminosity from the sensor (lux is the luminosity taken in "lux" measure units)
	// the last parameter can be 1 to enable library auto gain, or 0 to disable it
	rc = TSL2561_SENSELIGHT(&light1, &broadband, &ir, &lux, 1);
//	printf("Test. RC: %i(%s), broadband: %i, ir: %i, lux: %i\n", rc, strerror(light1.lasterr), broadband, ir, lux);
	
	TSL2561_CLOSE(&light1);
	
	
	
	tsl2561->broadband = broadband;
	tsl2561->ir = ir;
	tsl2561->lux = lux;
	
	return 0;
}

static void *log_db_thread_run(void*);
static void *server_thread_run(void*);







/* ==== Main program entry point ============================================ */

int main(int argc, char** argv) {
    cout << "Raspberry PI - Sensor Monitor" << endl;
    cout << "   2016, Felix Niederwanger <felix@feldspaten.org>" << endl;
    cout << "   University of Innsbruck, Austria" << endl << endl;
    
    htu21df_t htu21df;
    tsl2561_t tsl2561;
    char* log_file = NULL;
    const long long startTime = getSystemTime();
    long delay = 5;				// Delay between logging cycles
    vector<UdpServer*> udp;		// List of UDP servers where to push
    shm_t shmLocal;
    shm_memory = &shmLocal;
    
    // Parse program arguments
	for(int i=1;i<argc;i++) {
		string arg = string(argv[i]);
		const bool last = i >= argc-1;
		try {
			if(arg.length() == 0) continue;
			if(arg.at(0) == '-') {
			
				if(arg == "-h" || arg == "--help") {
					// Print help
					cout << "Usage: " << argv[0] << " [OPTIONS]" << endl;
					cout << "OPTIONS" << endl;
					cout << "  --nolog                Do not log to file (default)" << endl;
					cout << "  -f FILE  --log FILE    Write values to the given log file" << endl;
					cout << "  -d SEC   --delay SEC   Define seconds between logging intervals" << endl;
					cout << "  -i DEV   --i2c DEV     Define i2c device file" << endl;
					cout << "  --udp REMOTE           Send data to REMOTE via UDP. REMOTE must be an IP address" << endl;
					cout << "                         REMOTE can also be of the form IP:PORT if you want to use a custom port" << endl;
					cout << "  --srv PORT" << endl;
					cout << "    --server PORT        Eanble and set the port of the TCP server" << endl;
					cout << "                         It is possible to define more UDP servers to push" << endl;
					cout << "  --db FILE              Log to sqlite3 Database FILE" << endl;
					cout << "  --dbd DELAY            Database log delay in seconds" << endl;
					cout << "  --dblogdelay DELAY     Database log delay in seconds" << endl;
					cout << "  --shm ID               Attach shared memory" << endl;
					cout << "  --name NAME            Set the name of the station" << endl;
					cout << endl;
					return EXIT_SUCCESS;
				
				} else if (arg == "--nolog")			// No log
					log_file = NULL;
				else if (arg == "-f" || arg == "--log") {			// Define log file
					if (last) throw "Missing argument: Log file";
					log_file = argv[++i];
				} else if(arg == "-d" || arg == "--delay") {
					if (last) throw "Missing argument: Delay";
					delay = atol(argv[++i]);
					if (delay < 0) delay = 0;
				} else if(arg == "-i" || arg == "--i2c") {
					if(last) throw "Missing argument: i2c device file";
					// TODO: Implement me
					throw "i2c device file not yet implemented";
				} else if(arg == "--db") {
					if(last) throw "Missing argument: database file";
					database = new Database(argv[++i]);
				} else if(arg == "--dbd" || arg == "--dblogdelay") {
					if(last) throw "Missing argument: database log delay";
					db_log_delay = atol(argv[++i]);
					if(db_log_delay <= 0) db_log_delay = 1;
				} else if(arg == "--srv" || arg == "--server") {
					if(last) throw "Missing argument: Server port";
					srv_port = atoi(argv[++i]);
					if(srv_port < 0) throw "Illegal server port";
				} else if(arg == "--shm") {
					if(last) throw "Missing argument: shared memory id";
					shm_id = atoi(argv[++i]);
					if(shm_id <= 0)
						shm_id = 0;
					else
						cout << "  Shared memory ID: " << shm_id << endl;
				} else if(arg == "--name") {
					if(last) throw "Missing argument: Name";
					name = string(argv[++i]);
					if(name.length() == 0) {
						cerr << "WARNING: Empty station name not allowed";
						name = "meteo";
					}
				} else if(arg == "--udp") {
					if(last) throw "Missing argument: UDP remote";
					string remote = string(argv[++i]);
					try {
						int port = 7443;
					
						size_t index = remote.find(":");
						if(index != std::string::npos) {
							string sPort = remote.substr(index+1);
							remote = remote.substr(0, index-1);
							port = atoi(sPort.c_str());
						}
				
						UdpServer *server = new UdpServer(remote, port);
						udp.push_back(server);
					
						cout << "  Pushing to UDP:" << remote << ":" << port << endl;
					
					} catch (const char* msg) {
						cerr << "Error setting up UDP: " << msg << endl;
						return EXIT_FAILURE;
					} catch (char* msg) {
						cerr << "Error setting up UDP: " << msg << endl;
						return EXIT_FAILURE;
					} catch (...) {
						cerr << "Unknown error setting up UDP" << endl;
						return EXIT_FAILURE;
					}
				} else
					throw "Illegal program argument";
			
			} else
				throw "Illegal program argument";
				
		} catch (const char* msg) {
			cerr << "Invalid program arguments: " << msg << " " << arg << endl;
			cerr << "  Use " << argv[0] << " --help, if you need help." << endl;
			return EXIT_FAILURE;
		}
	}
	
	// Startup. Register signal handlers and cleanup routine 	
 	signal(SIGTERM, sig_handler);
 	signal(SIGINT, sig_handler);
 	signal(SIGUSR1, sig_handler);
 	atexit(cleanup);
	
    
    // Setup shared memory
    shm_memory->timestamp = 0;
    shm_memory->state = STATE_SETUP;		// Setup
    shm_memory->temperature = 0.0F;
    shm_memory->humidity = 0.0F;
    shm_memory->lux_broadband = 0.0F;
    shm_memory->lux_visible = 0.0F;
    shm_memory->lux_ir = 0.0F;
    if(shm_id > 0) {
    	// Setup shared memory segment
    	key_t id = shm_id;
    	shm_id = shmget(id, sizeof(shm_t), 0664 | IPC_CREAT);
    	if(shm_id < 0) {
    		cerr << "Error " << errno << " creating shared memory: " << strerror(errno) << endl;
    		return EXIT_FAILURE;
    	}
    	shm_memory = (shm_t*)shmat(shm_id, (void *)0, 0);
    	if(shm_memory == NULL) {
    		cerr << "Attaching shared memory failed: " << strerror(errno) << endl;
    		return EXIT_FAILURE;
    	}
    	
    	shm_id = id;		// Reset to key
    }
    
    // Setup database, if desired
    if(database != NULL) {
    	database->setupStation(name);
    	cout << "  Logging to database " << database->filename() << " each " << db_log_delay << " seconds" << endl;
    	
    	// Start logging thread
    	int rc;
    	rc = pthread_create(&db_thread, NULL, log_db_thread_run, NULL);
    	if(rc != 0) {
    		cerr << "Error creating database thread: " << strerror(errno) << endl;
    		return EXIT_FAILURE;
    	}
    }
    
    // Setup server, if needed
    if(srv_port > 0) {
    	cout << "Starting up server at port " << srv_port << " ... " << endl;
    	int rc = pthread_create(&srv_thread, NULL, server_thread_run, NULL);
    	if(rc != 0) {
    		cerr << "Error creating server thread: " << strerror(errno) << endl;
    		return EXIT_FAILURE;
    	}
    }
    
 	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
    if(log_file != NULL) {
	    file = fopen(log_file, "a");
	 	
	 	if(file == NULL) {
	 		cerr << "Error opening file " << log_file << ": " << strerror(errno) << endl;
	 		return EXIT_FAILURE;
	 	}
	 	fprintf(file, "# Logging file for Monitor\n");
	 	fprintf(file, "# Time\tTemperature\tHumidity\tLux_Broadband\tLux_Lux\tLux_IR\n");
	 	fprintf(file, "# Start time = %lld\n\n", startTime);
 	
		fprintf(file, "# Now: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
 	}
	printf("Startup: %d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	cout << "; Station " << name << endl;


	// Probe sensors
	cout << "Probing sensors ... " << endl;
	{ 
    	int i2cfd = i2c_open(I2CDEV);
    	if(i2cfd < 0) {
    		cerr << "Error opening i2c bus: " << strerror(errno) << endl;
    		return EXIT_FAILURE;
    	}
    	i2c_close(i2cfd);
		
		// Probe sensors and store first values
    	int rc_htu21df;
    	int rc_tsl2561;
		rc_htu21df = read_htu21df(&htu21df);			// HTU21df Humidity-temperature
    	if(rc_htu21df == 0) {
    		shm_memory->temperature = htu21df.temperature;
			shm_memory->humidity = htu21df.humidity;
    	} else {
    		cerr << "Error reading HTU21DF sensor: " << strerror(errno) << endl;
    		return EXIT_FAILURE;
    	}
    	
    	rc_tsl2561 = read_tsl2561(&tsl2561);				// TSL2561 luminosity sensor
    	if(rc_tsl2561 == 0) {
			shm_memory->lux_broadband = tsl2561.broadband;
			shm_memory->lux_visible = tsl2561.lux;
			shm_memory->lux_ir = tsl2561.ir;
    	} else {
    		cerr << "Error reading TSL2561 sensor: " << strerror(errno) << endl;
    		return EXIT_FAILURE;
    	}
    	
    	// All good.
	}
	
	
	cout << "Sensors OK. Entering main routine" << endl;
    // While we are running
    while(running) {
    	int rc_htu21df;
    	int rc_tsl2561;
    	// Current offset from the start time
 		long long time = getSystemTime() - startTime;
    	
    	// Reset sensor data
    	htu21df.temperature = 0;
    	htu21df.humidity = 0;
    	tsl2561.broadband = 0;
    	tsl2561.lux = 0;
    	tsl2561.ir = 0;
    	
    	
    	
    	// Read sensors
    	rc_htu21df = read_htu21df(&htu21df);			// HTU21df Humidity-temperature
    	if(rc_htu21df == 0) {
    		cout << "Temperature:         " << htu21df.temperature << " deg C" << endl;
    		cout << "Humidity:            " << htu21df.humidity << " % rel" << endl;
    		
    	} else {
    		cerr << "Error reading HTU21DF sensor: " << strerror(errno) << endl;
    	}
    	
    	rc_tsl2561 = read_tsl2561(&tsl2561);				// TSL2561 luminosity sensor
    	if(rc_tsl2561 == 0) {
    		cout << "Luminosity:          " << tsl2561.broadband << "\t" << tsl2561.lux << "\t" << tsl2561.ir << endl;
    		
    	} else {
    		cerr << "Error reading TSL2561 sensor: " << strerror(errno) << endl;
    	}
 
 		// Refresh shared memory
		shm_memory->state = STATE_READ;		// Currently reading
		shm_memory->timestamp = (long)(getSystemTime()/1000LL);
 		if(rc_htu21df == 0) {
			shm_memory->temperature = shm_memory->temperature * ALPHA + (1.0-ALPHA) * htu21df.temperature;
			shm_memory->humidity = shm_memory->humidity * ALPHA + (1.0-ALPHA) * htu21df.humidity;
 		}
 		if(rc_tsl2561 == 0) {
			shm_memory->lux_broadband = ALPHA * shm_memory->lux_broadband + (1.0-ALPHA) * tsl2561.broadband;
			shm_memory->lux_visible = ALPHA * shm_memory->lux_visible + (1.0-ALPHA) * tsl2561.lux;
			shm_memory->lux_ir = ALPHA * shm_memory->lux_ir + (1.0-ALPHA) * tsl2561.ir;
 		}
 		// OK
 		shm_memory->state = STATE_OK;		// OK
 
 
 
 		// Write to log file, if desired
 		if(file != NULL) {
 			fprintf(file, "%ld\t%f\t%f\t%f\t%f\t%f\n", ((long)time/1000L), htu21df.temperature, htu21df.humidity, 
 				tsl2561.broadband, tsl2561.lux, tsl2561.ir);
 			fflush(file);
 		}
    	
    	// Send via UDP, if desired
    	if(udp.size() > 0) {
    		// Build packet
    		stringstream ss;
    		
    		ss << (getSystemTime()/1000LL) << "," << time << "," << name << "\n";
    		ss << htu21df.temperature << "," << htu21df.humidity << "\n";
    		ss << tsl2561.broadband << "," << tsl2561.lux << "," << tsl2561.ir;
    		
    		// Send on each server
    		for(vector<UdpServer*>::iterator it = udp.begin(); it!=udp.end(); ++it) {
    			// Try to send all the data
				try {
					(*it)->send(ss.str());
				} catch(const char* msg) {
					cerr << "UDP send failed: " << msg << endl;
				} catch(char* msg) {
					cerr << "UDP send failed: " << msg << endl;
				} catch(...) {
					cerr << "UDP send failed: Unknown error" << endl;
				}
			}
    	}
    	
    	// Sleep until next cycle
    	if(delay > 0)
	    	sleep(delay);
    }
    if(file != NULL)
		fclose(file);
	for(vector<UdpServer*>::iterator it = udp.begin(); it!=udp.end(); ++it) {
		UdpServer *udp = *it;
		udp->close();
		delete udp;
	}
	udp.clear();
    
    
    cout << "Bye" << endl;
    return EXIT_SUCCESS;
}

static void *server_thread_run(void*) {
	if(srv_port <= 0) return 0;
	
	server.setPort(srv_port);
	server.setShm(shm_memory);
	try {
		server.start();
	} catch (const char* msg) {
		cerr << "Server error: " << msg << endl;
	}
	return 0;
}

static void *log_db_thread_run(void*) {
	string table = name;
	
	// Wait
	sleep(db_log_delay);
	
	// Iterate
	int warnings = 0;		// Prevent more warning messages
	while(running) {
		long long delay = getSystemTime() + db_log_delay*1000L;
		
		
		shm_t memory;
		memcpy(&memory, shm_memory, sizeof(shm_t));
		
		// Check if ready
		if(memory.state == STATE_OK) {
			warnings = 0;
			
			if(database == NULL) {
				cerr << "ERROR: Database is NULL" << endl;
				return 0;
			}
			
			
			int retries = 5;			// At most 5 write tries
			
			nocleanup = true;
			while(retries-- > 0) {
				bool success = false;
				try {
					database->write(table, memory.temperature, memory.humidity, memory.lux_broadband, memory.lux_visible, memory.lux_ir);
					cout << "Dataset written to database: " <<
						memory.temperature << " deg C, " << memory.humidity << " % rel, " <<
						" Broadband: " << memory.lux_broadband << ", Visible: " << memory.lux_visible << 
						", IR: " << memory.lux_ir << endl;
					success = true;
					break;
				} catch (...) {
					cerr << "DB write failed: " << database->getErrorMessage() << endl;
				}
				
				// If not successfull, try again
				if(success) break;
				else
					sleep(1);
			}
			nocleanup = false;
			
		} else {
			if(warnings == 0) 
				cerr << "WARN: Sensors not ready. Not writing to database" << endl;
			else if(warnings <= 3) 
				cerr << "WARN: Sensors not ready (" << warnings << " times). Not writing to database" << endl;
			warnings++;
		}
		
		
		
		
		
		delay -= getSystemTime();
		delay = ceil(delay/1000.0);		// To seconds
		if(delay > 0) sleep(delay);
	}
	
	
	return 0;
}

