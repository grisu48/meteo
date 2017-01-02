/* =============================================================================
 * 
 * Title:         Meteo Main program
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>


#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>

// Include sensors
#include "sensors.hpp"
#include "config.hpp"


using namespace std;
using namespace sensors;
using namespace meteo;

// Sensor enable switches (1 = ENABLED, 0 = DISABLED)
#define SENSOR_BMP180 1
#define SENSOR_HTU21DF 1
#define SENSOR_MCP9808 1
#define SENSOR_TSL2561 1

#define DEFAULT_UDP_PORT 5232

/* ==== CLASS DECLARATIONS ================================================== */

class UdpBroadcast {
private:
	int fd;
	struct sockaddr_in dest_addr;
	int port;
	std::string remote;
	
public:
	UdpBroadcast(std::string remote, int port);
	virtual ~UdpBroadcast();
	
	std::string getRemoteHost(void) const { return this->remote; }
	int getPort(void) { return this->port; }
	
	/** Close this broadcast instance */
	void close(void);
	/** Send the given message */
	ssize_t send(const char* msg, size_t size);
	/** Send the given message */
	ssize_t send(std::string msg);	
};

class TcpBroadcast {
private:
	string remote;
	int port;
	struct sockaddr_in m_addr;
	
public:
	TcpBroadcast(string remote, int port) {
		this->remote = remote;
		this->port = port;
		
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
	
		if(inet_pton(AF_INET, this->remote.c_str(), &m_addr.sin_addr)<=0)
			throw "Illegal remote TCP-IP address";
	}
	
	string getRemote(void) const { return remote; }
	int getPort(void) const { return port; }
	
	/** Send the given message */
	ssize_t send(const char* msg, size_t size);
	/** Send the given message */
	ssize_t send(std::string msg);	
};


UdpBroadcast::UdpBroadcast(std::string remote, int port) {
	this->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(this->fd < 0) throw "Error setting up socket";
	int broadcastEnable=1;
	int ret=setsockopt(this->fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
	if(ret != 0) throw "Error enabling broadcast";	

	this->port = port;
	this->remote = remote;
	
	// Setup destination address
	memset(&dest_addr, '\0', sizeof(struct sockaddr_in));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = (in_port_t)htons(port);
	//dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	dest_addr.sin_addr.s_addr = inet_addr(remote.c_str());
}

UdpBroadcast::~UdpBroadcast() {
	this->close();
}


void UdpBroadcast::close() {
	if(this->fd > 0) ::close(this->fd);
	this->fd = 0;
}


ssize_t UdpBroadcast::send(const char* msg, size_t size) {
	ssize_t ret;
	int flags = 0;
	
	const socklen_t dest_len = sizeof(dest_addr);
	
	
	ret = sendto(this->fd, (void*)msg, sizeof(char)*size, flags, (const sockaddr*)&dest_addr, dest_len);
	return ret;
}

ssize_t UdpBroadcast::send(std::string msg) {
	return this->send(msg.c_str(), msg.length());
	
	
	return -1;
}





ssize_t TcpBroadcast::send(const char* msg, size_t size) {
	if(msg == NULL || size == 0) return 0L;
	
	const int sock = ::socket( AF_INET, SOCK_STREAM, 0);
	if(sock < 0) return -1L;
	
	if(::connect(sock, (struct sockaddr*)&m_addr, sizeof(m_addr)) < 0) {
		::close(sock);
		return -1L;
	} else {
		const int flags = 0;
		ssize_t ret = ::send(sock, msg, sizeof(char)*size, flags);
		::close(sock);
		return ret;
	}
	
	return 0L;
}

ssize_t TcpBroadcast::send(std::string msg) {
	return this->send(msg.c_str(), msg.size());
}



struct {
	bool enabled_BMP180 = false;
	bool enabled_HTU21DF = false;
	bool enabled_MCP9808 = false;
	bool enabled_TSL2561 = false;
	bool daemonize = false;
	int station_id = 0;
	string i2c_device = std::string(Sensor::DEFAULT_I2C_DEVICE);
	long delay = 1000L;		// Delay between iterations in milliseconds
	int reportEvery = 0;	// Report every N iterations 	XXX: NOT YET IMPLEMENTED
} typedef config_t;

/* ==== END OF CLASS DECLARATIONS =========================================== */

// Global running flag
static volatile bool running = true;

// XXX: Don't use static variables, you lazy! :-)


static vector<UdpBroadcast> broadcasts;
static vector<TcpBroadcast> tcp_broadcasts;
/** Static meteo configuration */
static config_t config;



static bool probeSensor(Sensor& sensor);
static void probeSensors(void);
static void printHelp(const char* progname);
static void sig_handler(int sig_no);
static void addBroadcast(const char* address);
static void addTcpBroadcast(const char* address);
static int64_t getSystemTime(void);
/** Precision sleep */
static int p_sleep(long millis, long micros);
static int p_sleep(long millis) { return p_sleep(millis, 0); }
/** Apply the given config file */
static void applyConfig(Config &config);
/** Rung program as deamon */
static void deamonize();

/* ==== MAIN ================================================================ */

int main(int argc, char** argv) {
	bool verbose = false;
	bool readConfig = true;		// Read configuration
	
	// First check, if the configuration is skipped	
	for(int i=1;i<argc;i++) {
		string arg = string(argv[i]);
		if(arg == "--noconfig") {
			readConfig = false;
		} else if(arg == "-v" || arg == "--verbose") {
			verbose = true;
		}
	}
	// Read parameters from config file (s)
	if (readConfig) {
		if (verbose) cout << "Reading configuration files ... " << endl;
		Config config("/etc/meteo.cf");
		applyConfig(config);
		config.readFile("./meteo.cf");
		applyConfig(config);
	} else {
		if(verbose) cerr << "Config files skipped (--noconfig)" << endl;
	}

	// Parse all arguments
	try {
		for(int i=1;i<argc;i++) {
			string arg = string(argv[i]);
			const bool isLast = i >= argc-1;
			if (arg.length() == 0) continue;
			if(arg.at(0) == '-') {
			
				if( arg == "-h" || arg == "--help") {
					printHelp(argv[0]);
					return EXIT_SUCCESS;
				} else if(arg == "--probe") {
					probeSensors();
					return EXIT_SUCCESS;
				} else if(arg == "--id") {
					if(isLast) throw "Missing argument: Station id";
					config.station_id = atoi(argv[++i]);
				} else if(arg == "--udp") {
					if(isLast) throw "Missing argument: Broadcast address";
					addBroadcast(argv[++i]);
				} else if(arg == "--tcp") {
					if(isLast) throw "Missing argument: TCP destination address";
					addTcpBroadcast(argv[++i]);
				} else if(arg == "-d" || arg == "--delay") {
					if(isLast) throw "Missing argument: Delay time";
					config.delay = atol(argv[++i]);
					if(config.delay <= 0) throw "Illegal delay time";
				} else if(arg == "--daemon") {
					config.daemonize = true;
				} else if(arg == "--i2c" || arg == "--device") {
					if(isLast) throw "Missing argument: i2c device";
					config.i2c_device = std::string(argv[++i]);
				} else if(arg == "-v" || arg == "--verbose" || arg == "--noconfig") {
					// Already processed
				} else if(arg == "--bmp180") {
					config.enabled_BMP180 = true;
				} else if(arg == "--htu21df") {
					config.enabled_HTU21DF = true;
				} else if(arg == "--mcp9808") {
					config.enabled_MCP9808 = true;
				} else if(arg == "--tsl2561") {
					config.enabled_TSL2561 = true;
				} else if(arg == "-a" || arg == "--all") {
					config.enabled_BMP180 = true;
					config.enabled_HTU21DF = true;
					config.enabled_MCP9808 = true;
					config.enabled_TSL2561 = true;
				} else {
					cerr << "Invalid program argument: " << arg << endl;
					cout << "Type " << argv[0] << " --help, if you need help" << endl;
					return EXIT_FAILURE;
				}
			
			}
		}
	} catch (const char* msg) {
		cerr << "Error in program parameters: " << msg << endl;
		cout << "Type " << argv[0] << " --help if you need help" << endl;
		return EXIT_FAILURE;
	}
	
	
	// Daemonize, if needed
	if(config.daemonize) {
		deamonize();
		cout << "Running as daemon" << endl;
	}
	
	
	// Register signal handlers
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    
    if(verbose) {
    	if(broadcasts.size() > 0) {
    		cout << broadcasts.size() << " udp broadcast destination(s)" << endl;
	    	for(vector<UdpBroadcast>::iterator it=broadcasts.begin(); it != broadcasts.end(); it++)
	    		cout << "  " << (*it).getRemoteHost() << ":" << (*it).getPort() << endl;
	    }
    }
    
    // Try to access the device file
	{
		int fd = open(config.i2c_device.c_str(), O_RDONLY, 0);
		if (fd < 0) cerr << "WARNING: Error accessing device file '" << config.i2c_device << "': " << strerror(errno) << endl;
		else close(fd);
	}
    

    // Setting up sensors
	int sensor_count = 0;
#if SENSOR_BMP180 == 1
	BMP180 bmp180(config.i2c_device);
	if (config.enabled_BMP180) sensor_count++;	
#endif
#if SENSOR_HTU21DF == 1
	HTU21DF htu21df(config.i2c_device);
	if (config.enabled_HTU21DF) sensor_count++;
#endif
#if SENSOR_MCP9808 == 1
	MCP9808 mcp9808(config.i2c_device);
	if (config.enabled_MCP9808) sensor_count++;
#endif
#if SENSOR_TSL2561 == 1
	TSL2561 tsl2561(config.i2c_device);
	if (config.enabled_TSL2561) sensor_count++;
#endif
    
    if(sensor_count == 0) {
    	cerr << "Warning: No sensors present" << endl;
    }
    
    if(verbose) cout << "Enter main routine ... " << endl;
    stringstream buffer;
    const long delay = config.delay;
    int iteration = 0;		// Iteration until reporting
    while(running) {
    	long time = -getSystemTime();
    	
    	buffer.str("");		// Clear buffer
    	buffer << "<meteo station=\"" << config.station_id << "\"";
    	buffer << " timestamp=\"" << getSystemTime() << "\"";
    	
    	// Determine also average temperature
    	vector<double> avg_temperature;
    	
	int ret;
#if SENSOR_BMP180 == 1
		if (config.enabled_BMP180) {
			ret = bmp180.read();
			buffer << " bmp180=\"" << ret << "\" temperature_bmp180=\"" << bmp180.temperature() << "\" pressure=\"" << bmp180.pressure() << "\"";
			if (ret == 0)
				avg_temperature.push_back(bmp180.temperature());
		}
#endif
#if SENSOR_HTU21DF == 1
		if (config.enabled_HTU21DF) {
			ret = htu21df.read();
			buffer << " htu21df=\"" << ret << "\" temperature_htu21df=\"" << htu21df.temperature() << "\" humidity=\"" << htu21df.humidity() << "\"";
			if (ret == 0)
				avg_temperature.push_back(htu21df.temperature());
		}
#endif
#if SENSOR_MCP9808 == 1
		if (config.enabled_MCP9808) {
			ret = mcp9808.read();
			buffer << " mcp9808=\"" << ret << "\" temperature_mcp9808=\"" << mcp9808.temperature() << "\"";
			if(ret == 0)
				avg_temperature.push_back(mcp9808.temperature());
		}
#endif
#if SENSOR_TSL2561 == 1
		 if (config.enabled_TSL2561) {
			ret = tsl2561.read();
			buffer << " tsl2561=\"" << ret << "\" lux_visible=\"" << tsl2561.visible() << "\" lux_infrared=\"" << tsl2561.ir() << "\"";
		}
#endif
    	
    	
		if(avg_temperature.size() > 0) {
			double temp = 0.0;
			int count = 0;
			for(vector<double>::iterator it = avg_temperature.begin(); it != avg_temperature.end(); ++it) {
				temp += *it;
				count += 1;
			}
			if(count > 0) {
				temp /= (double)count;
				buffer << " temperature=\"" << temp << "\"";
			}
		}
    	
    	buffer << "/>";
    	
    	// Broadcast to other clients
    	if(iteration++ > config.reportEvery) {
    		iteration = 0;
    	
			string message = buffer.str() + "\n";
			cout << message;
			ssize_t ret;
	    	for(vector<UdpBroadcast>::iterator it=broadcasts.begin(); it != broadcasts.end(); it++)
    			(*it).send(message);
    		for(vector<TcpBroadcast>::iterator it=tcp_broadcasts.begin(); it != tcp_broadcasts.end(); it++) {
    			ret = (*it).send(message);
    			if(ret < 0)
    				cerr << "Error sending data via tcp to " << (*it).getRemote() << ":" << (*it).getPort()<< " - " << strerror(errno) << endl;
    		}
    	}
    	
    	
    	// Sleep adequate amount of time
    	time += getSystemTime();
    	time = delay - time;
    	if(time > 0L) {
    		int ret = p_sleep(time);
    		if(ret == -2 && !running) break;		// Interrupted and break!
    		
    		if(ret != 0) {
    			// Fallback: Default old sleep
    			cerr << "p_sleep(" << time << ") returned status " << ret;
    			ret = (time / 1000L);
    			if(ret > 0) {
    				cerr << ". Fallback to sleep(" << ret << ")" << endl;
    				sleep(ret);
    			}
    			cerr << endl;
    		}
    	}
    }
    
    cout << "Bye" << endl;
    return EXIT_SUCCESS;
}


/* ==== END OF MAIN ========================================================= */











static void deamonize(void) {
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

static bool probeSensor(Sensor& sensor) {
	if(sensor.read() != 0) return false;
	return !sensor.isError();
}

static void probeSensors(void) {
	const char* i2c_device = config.i2c_device.c_str();
	
	// Probe all sensors
	cout << " Probing BMP180 ...  "; cout.flush();
    BMP180 bmp(i2c_device);
    if(probeSensor(bmp)) {
    	cout << "OK (" << bmp.temperature() << " deg. C; " << bmp.pressure() << " hPa, " << bmp.altitude() << " m)" << endl;
    } else {
    	cout << "Error" << endl;
    }
    
    cout << " Probing HTU21DF ... "; cout.flush();
    HTU21DF htu21df(i2c_device);
    if(probeSensor(htu21df)) {
    	cout << "OK (" << htu21df.temperature() << " deg. C; " << htu21df.humidity() << " % rel.)" << endl;
    } else {
    	cout << "Error" << endl;
    }
    
    cout << " Probing MCP9808 ... "; cout.flush();
    MCP9808 mcp9808(i2c_device);
    if(probeSensor(mcp9808)) {
    	cout << "OK (" << mcp9808.temperature() << " deg. C)" << endl;
    } else {
    	cout << "Error" << endl;
    }
    
    cout << " Probing TSL2561 ... "; cout.flush();
    TSL2561 tsl2561(i2c_device);
    if(probeSensor(tsl2561)) {
    	cout << "OK (" << tsl2561.visible() << " visible; " << tsl2561.ir() << " infrared)" << endl;
    } else {
    	cout << "Error" << endl;
    }
}

static void printHelp(const char* progname) {
	// THIS are 80 characters
	//      "                                                                                "
	cout << "meteo - Enviromental monitoring" << endl;
	cout << "  2016, Felix Niederwanger" << endl << endl;
	cout << "Usage: " << progname << " [OPTIONS]" << endl;
	cout << "OPTIONS" << endl;
	cout << "  -h    --help              Display this help message" << endl;
	cout << "  -v    --verbose           Verbosity mode" << endl;
	cout << "        --id ID             Manually set station id to ID" << endl;
	cout << "        --noconfig          Do not read config files" << endl;
	cout << "        --udp REMOTE:PORT   Set endpoint for UDP broadcast" << endl;
	cout << "                            Definition of multiple endpoints is possible by" << endl;
	cout << "                            Repeating the argument" << endl;
	cout << "        --tcp REMOTE:PORT   Set endpoint for TCP data sent" << endl;
	cout << "        --probe             Probe all supported sensors" << endl;
	cout << "        --bmp180            Enable BMP180 sensor (Pressure sensor)" << endl;
	cout << "        --htu21df           Enable HTU21D-F sensor (Temperatue + Humidity)" << endl;
	cout << "        --mcp9808           Enable MCP9808 sensor (High accuracy temperature)" << endl;
	cout << "        --tsl2561            nable TSL2561 sensor (Luminosity sensor)" << endl;
	cout << "  -a    --all               Enable all sensors" << endl;
	
	
	cout << endl;
	cout << "CONFIG FILES" << endl;
    cout << "Before applying program paremeters the following two files are read" << endl;
    cout << "  * /etc/meteo.cf" << endl;
    cout << "  * ./meteo.cf" << endl;
    cout << "/etc/meteo.cf give a global configuration, ./meteo.cf a local user configuration" << endl;
    cout << "  and the program arguments overwrite them again for even more local settings" << endl;
}

static void sig_handler(int sig_no) {
	switch(sig_no) {
		case SIGTERM:
		case SIGINT:
			if(!running) {
				cerr << "Terminating" << endl;
				exit(EXIT_FAILURE);
			} else {
				cerr << "Termination request" << endl;
				running = false;
			}
	}
}


static void addBroadcast(const char* addr) {
	string address = string(addr);
	int port = DEFAULT_UDP_PORT;
	size_t sIndex = address.find(":");
	if(sIndex != string::npos) {
		string sPort = address.substr(sIndex+1);
		address = address.substr(0,sIndex);
		port = atoi(sPort.c_str());
	}
	
	if (port <= 0) throw "Illegal port";
	if (address.length() == 0) throw "Missing address";
	
	// Create and add broadcast
	broadcasts.push_back( UdpBroadcast(address, port) );
}


static void addTcpBroadcast(const char* addr) {
	string address = string(addr);
	int port = DEFAULT_UDP_PORT;
	size_t sIndex = address.find(":");
	if(sIndex != string::npos) {
		string sPort = address.substr(sIndex+1);
		address = address.substr(0,sIndex);
		port = atoi(sPort.c_str());
	}
	
	if (port <= 0) throw "Illegal port";
	if (address.length() == 0) throw "Missing address";
	
	tcp_broadcasts.push_back( TcpBroadcast(address, port) );
}

static int64_t getSystemTime(void) {
	struct timeval tv;
	int64_t result;
	gettimeofday(&tv, 0);
	result = tv.tv_sec;
	result *= 1000L;
	result += (int64_t)tv.tv_usec/1000L;
	return result;
}



static int p_sleep(long millis, long micros) {
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
			// Note: In "rem" the remaining time is stored
			return -2;
		case EINVAL:
			return -3;
		default :
			return -8;
		}
	} else 
		return 0;
}

static void applyConfig(Config &conf) {
	config.station_id = conf.getInt("id", config.station_id);
	
	config.enabled_BMP180 = conf.getBoolean("bmp180", config.enabled_BMP180);
	config.enabled_HTU21DF = conf.getBoolean("htu21df", config.enabled_HTU21DF);
	config.enabled_MCP9808 = conf.getBoolean("mcp9808", config.enabled_MCP9808);
	config.enabled_TSL2561 = conf.getBoolean("tsl2561", config.enabled_TSL2561);
	config.daemonize = conf.getBoolean("daemonize", config.daemonize);
	
	// Broadcasts 
	string temp = conf.get("tcp");
	if(temp.size() > 0) {
		try {
			addTcpBroadcast(temp.c_str());
		} catch(const char *msg) {
			cerr << "Error adding tcp broadcast: " << temp << " - " << msg << endl;
		}
	}
	temp = conf.get("udp");
	if(temp.size() > 0) {
		try {
			addBroadcast(temp.c_str());
		} catch(const char *msg) {
			cerr << "Error adding udp broadcast: " << temp << " - " << msg << endl;
		}
	}
	
	// I2C device
	temp = conf.get("i2c");
	if(temp.size() > 0) {
		config.i2c_device = temp;
	}
}

