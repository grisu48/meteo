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



// Sensor enable switches (1 = ENABLED, 0 = DISABLED)
#define SENSOR_BMP180 1
#define SENSOR_HTU21DF 1
#define SENSOR_MCP9808 1
#define SENSOR_TSL2561 1

#define DEFAULT_UDP_PORT 5232

/* ==== CLASS DECLARATIONS ================================================== */

/** Class for sending data over UDP */
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
	int getPort(void) const { return this->port; }
	
	/** Close this broadcast instance */
	void close(void);
	/** Send the given message */
	ssize_t send(const char* msg, size_t size);
	/** Send the given message */
	ssize_t send(std::string msg);	
};

/** Class for sending the data over TCP */
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
	this->port = port;
	this->remote = remote;
	
	// Setup destination address
	memset(&this->dest_addr, '\0', sizeof(this->dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = (in_port_t)htons(port);
	dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
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
	
	const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0) return -1;
	
	int broadcastEnable=1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
		
	ret = ::sendto(fd, (void*)msg, sizeof(char)*size, flags, (const sockaddr*)&dest_addr, dest_len);
	::close(fd);
	return ret;
}

ssize_t UdpBroadcast::send(std::string msg) {
	return this->send(msg.c_str(), msg.length());
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


/** Socket helper */
class Socket {
private:
	int sock;
	
	ssize_t write(const void* buf, size_t len) {
		if(sock <= 0) throw "Socket closed";
		ssize_t ret = ::send(sock, buf, len, 0);
		if(ret == 0) {
			this->close();
			throw "Socket closed";
		} else if (ret < 0) 
			throw strerror(errno);
		else
			return ret;
	}
	
public:
	Socket(int sock) { this->sock = sock; }
	virtual ~Socket() { this->close(); }
	void close() {
		if(sock > 0) {
			::close(sock);
			sock = 0;
		}
	}
	
	void print(const char* str) {
	    const size_t len = strlen(str);
		if(len == 0) return;
		this->write((void*)str, sizeof(char)*len);
	}
	
	void println(const char* str) {
	    this->print(str);
		this->write("\n", sizeof(char));
	}
	
	void print(const string &str) {
	    const size_t len = str.size();
		if(len == 0) return;
		this->write((void*)str.c_str(), sizeof(char)*len);
	}
	
	void println(const string &str) {
	    this->print(str);
		this->write("\n", sizeof(char));
	}
	
	Socket& operator<<(const char c) {
		this->write(&c, sizeof(char));
		return (*this);
	}
	Socket& operator<<(const char* str) {
		print(str);
		return (*this);
	}
	Socket& operator<<(const string &str) {
		this->print(str);
		return (*this);
	}
	
	Socket& operator<<(const int i) {
	    string str = ::to_string(i);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator<<(const long l) {
	    string str = ::to_string(l);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator<<(const float f) {
	    string str = ::to_string(f);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator<<(const double d) {
	    string str = ::to_string(d);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator>>(char &c) {
		if(sock <= 0) throw "Socket closed";
		ssize_t ret = ::recv(sock, &c, sizeof(char), 0);
		if(ret == 0) {
			this->close();
			throw "Socket closed";
		} else if (ret < 0) 
			throw strerror(errno);
		else
			return (*this);
	}
	
	Socket& operator>>(String &str) {
		String line = readLine();
		str = line;
		return (*this);
	}
	
	String readLine(void) {
		if(sock <= 0) throw "Socket closed";
		stringstream ss;
		while(true) {
			char c;
			ssize_t ret = ::recv(sock, &c, sizeof(char), 0);
			if(ret == 0) {
				this->close();
				throw "Socket closed";
			} else if (ret < 0) 
				throw strerror(errno);
				
			if(c == '\n')
				break;
			else
				ss << c;
		}
		String ret(ss.str());
		return ret.trim();
	}
	
	void writeHttpHeader(const int statusCode = 200) {
            this->print("HTTP/1.1 ");
            this->print(::to_string(statusCode));
            this->print(" OK\n");
            this->print("Content-Type:text/html\n");
            this->print("\n");
	}
};

class Average {
private:
	vector<double> values;
	
public:
	Average() {};
	virtual ~Average() {};
	
	Average &operator<<(const double v) {
		this->values.push_back(v);
		return (*this);
	}
	
	double average(void) const {
		const size_t len = this->values.size();
		if(len == 0) return 0.0;
		double ret = 0.0;
		for(vector<double>::const_iterator it = values.begin(); it != values.end(); ++it)
			ret += *it;
		return ret /(double)len;
	}
	
	size_t size(void) const { return this->values.size(); }
	
	void print(std::ostream &out = cout) {
		const size_t len = size();
		if(len == 0) {
			out << "-";
		} else if (len==1) {
			out << values[0];
		} else {
			out << average() << " (";
			bool first = true;
			for(vector<double>::const_iterator it = values.begin(); it != values.end(); ++it) {
				if(first) first = false;
				else out << ", ";
				out << *it;
			}
			out << ")";
		}
	}
};

class MeteoMosquitto {
public:
	MeteoMosquitto() {}
	virtual ~MeteoMosquitto() {}
	Mosquitto mosquitto;
	string topic = "meteo/raspberry";
	int id = -1;
	
	void publish(const map<string, float> &values, const map<string, int> &status) {
		// Build JSON package
		stringstream ss;
		
		ss << "{\"node_id\":" << id;
		for(map<string,int>::const_iterator it = status.begin(); it != status.end(); ++it) {
			ss << " ,";
			ss << "\"" << it->first << "\":" << it->second;
		}
		for(map<string,float>::const_iterator it = values.begin(); it != values.end(); ++it) {
			ss << " ,";
			ss << "\"" << it->first << "\":" << it->second;
		}
		
		ss << "}";
		
		string packet = ss.str();
		mosquitto.publish(topic, packet);
	}
};

struct {
	bool enabled_BMP180 = false;
	bool enabled_HTU21DF = false;
	bool enabled_MCP9808 = false;
	bool enabled_TSL2561 = false;
	bool daemonize = false;
	int station_id = 0;
	string i2c_device = std::string(Sensor::DEFAULT_I2C_DEVICE);
	long delay = 1000L;		// Delay between iterations in milliseconds
	int reportEvery = 0;	// Report every N iterations
	bool display = false;	// Display measurement on screen
} typedef config_t;

/* ==== END OF CLASS DECLARATIONS =========================================== */

// Global running flag
static volatile bool running = true;

/** All udp-broadcast instances */
static vector<UdpBroadcast> broadcasts;
/** All attached tcp-broadcast instances */
static vector<TcpBroadcast> tcp_broadcasts;
/** Mosquitto instances */
static vector<MeteoMosquitto*> mosquittos;
/** Static meteo configuration */
static config_t config;

/** List of all child processes */
static vector<pid_t> children;


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
/** Create a listening IPv4 socket */
static int createListeningSocket4(const int port);
/** Cleanup procedure at end */
static void cleanup();
/** Detach a webserver. Return child pid */
static pid_t startWebserver(const int port);
/** Read sensor with fault tolerance */
static int readSensor(Sensor &sensor);

/* ==== MAIN ================================================================ */

int main(int argc, char** argv) {
	bool verbose = false;
	bool silent = false;
	bool readConfig = true;		// Read configuration
	vector<int> httpPorts;		// Ports where the webserver will be launched
	
	// First check, if the configuration is skipped	
	for(int i=1;i<argc;i++) {
		string arg = string(argv[i]);
		if(arg == "--noconfig") {
			readConfig = false;
		} else if(arg == "-v" || arg == "--verbose") {
			silent = false;
			verbose = true;
		} else if(arg == "--silent") {
			verbose = false;
			silent = true;
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
		bool displaySet = false;
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
					
				} else if(arg == "--display") {
					config.display = true;
					displaySet = true;
				} else if(arg == "--daemon") {
					config.daemonize = true;
				} else if(arg == "--i2c" || arg == "--device") {
					if(isLast) throw "Missing argument: i2c device";
					config.i2c_device = std::string(argv[++i]);
				} else if(arg == "-v" || arg == "--verbose" || arg == "--noconfig" || arg == "--silent") {
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
				} else if(arg == "-H" || arg == "--http") {
					if(isLast) throw "Missing argument: Webserver port";
					const int port = ::atoi(argv[++i]);
					if(port <= 0) throw "Illegal port";
					httpPorts.push_back(port);
				} else {
					cerr << "Invalid program argument: " << arg << endl;
					cout << "Type " << argv[0] << " --help, if you need help" << endl;
					return EXIT_FAILURE;
				}
			
			}
		}
		
		// Enable display, if not daemon
		if(!displaySet) {
			if(!config.daemonize) config.display = true;
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
	
    
    // Starting Webservers. This must happend before registering signal handlers
    for(vector<int>::const_iterator it = httpPorts.begin(); it != httpPorts.end(); ++it) {
    	if(verbose) { cout << "Starting webserver (" << *it << ") ... "; cout.flush(); }
    	try {
	    	static pid_t pid = startWebserver(*it);
	    	if(verbose) { cout << pid << endl; cout.flush(); }
	    	if(pid > 0) 
	    		children.push_back(pid);
	    } catch (const char* msg) {
	    	cerr << "Error starting webserver: " << msg << endl;
	    	return EXIT_FAILURE;
	    }
    }
	
	// Register signal handlers
	atexit(cleanup);
    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    signal(SIGCHLD, sig_handler);
    
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
		Average temperature;
		Average humidity;
		Average pressure;
	
		map<string, float> data;
		map<string, int> status;
#if SENSOR_BMP180 == 1
		if (config.enabled_BMP180) {
			ret = readSensor(bmp180);
			buffer << " bmp180=\"" << ret << "\" temperature_bmp180=\"" << bmp180.temperature() << "\" pressure=\"" << bmp180.pressure() << "\"";
			status["bmp180"] = ret;
			if (ret == 0) {
				temperature << bmp180.temperature();
				pressure << bmp180.pressure();
				data["bmp180_t"] = bmp180.temperature();
				data["bmp180_p"] = bmp180.pressure();
			}
		}
#endif
#if SENSOR_HTU21DF == 1
		if (config.enabled_HTU21DF) {
			ret = readSensor(htu21df);
			buffer << " htu21df=\"" << ret << "\" temperature_htu21df=\"" << htu21df.temperature() << "\" humidity=\"" << htu21df.humidity() << "\"";
			status["htu21df"] = ret;
			if (ret == 0) {
				temperature << htu21df.temperature();
				humidity << htu21df.humidity();
				data["htu21df_t"] = htu21df.temperature();
				data["htu21df_h"] = htu21df.humidity();
			}
		}
#endif
#if SENSOR_MCP9808 == 1
		if (config.enabled_MCP9808) {
			ret = readSensor(mcp9808);
			status["mcp9808"] = ret;
			buffer << " mcp9808=\"" << ret << "\" temperature_mcp9808=\"" << mcp9808.temperature() << "\"";
			if(ret == 0) {
				temperature << mcp9808.temperature();
				data["mcp9808_t"] = mcp9808.temperature();
			}
		}
#endif
#if SENSOR_TSL2561 == 1
		 if (config.enabled_TSL2561) {
			ret = readSensor(tsl2561);
			status["tsl2561"] = ret;
			buffer << " tsl2561=\"" << ret << "\" lux_visible=\"" << tsl2561.visible() << "\" lux_infrared=\"" << tsl2561.ir() << "\"";
			if(ret == 0) {
				data["tsl2561_v"] = tsl2561.visible();
				data["tsl2561_ir"] = tsl2561.ir();
			}
		}
#endif
    	
    	if(temperature.size() > 0) {
    		double avg = temperature.average();
			buffer << " temperature=\"" << avg << "\"";
			data["temperature"] = avg;
		}
		if(humidity.size() > 0) {
    		double avg = humidity.average();
			buffer << " humidity=\"" << avg << "\"";
			data["humidity"] = avg;
		}
		if(pressure.size() > 0) {
    		double avg = pressure.average();
			buffer << " pressure=\"" << avg << "\"";
			data["pressure"] = avg;
		}
    	
    	buffer << "/>";
    	
    	// Broadcast to other clients
    	if(iteration++ > config.reportEvery) {
    		iteration = 0;
    	
			string message = buffer.str() + "\n";
			if(verbose) cout << message;
			else if (config.display && !silent) {
				// Pretty output
				if(temperature.size() > 0) { cout << "Temperature:  "; temperature.print(); cout << endl; }
				if(humidity.size() > 0) { cout << "Humidity:     "; humidity.print(); cout << endl; }
				if(pressure.size() > 0) { cout << "Pressure:     "; pressure.print(); cout << endl; }
			}
			ssize_t ret;
	    	for(vector<UdpBroadcast>::iterator it=broadcasts.begin(); it != broadcasts.end(); it++)
    			(*it).send(message);
    		for(vector<TcpBroadcast>::iterator it=tcp_broadcasts.begin(); it != tcp_broadcasts.end(); it++) {
    			ret = (*it).send(message);
    			if(ret < 0)
    				cerr << "Error sending data via tcp to " << (*it).getRemote() << ":" << (*it).getPort()<< " - " << strerror(errno) << endl;
    		}
    		if(mosquittos.size() > 0) {
    			for(vector<MeteoMosquitto*>::const_iterator it = mosquittos.begin(); it != mosquittos.end(); ++it)
    				(*it)->publish(data, status);
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
	cout << "        --silent            Disable verbose and be silent" << endl;
	cout << "        --id ID             Manually set station id to ID" << endl;
	cout << "        --noconfig          Do not read config files" << endl;
	cout << "        --udp REMOTE:PORT   Set endpoint for UDP broadcast" << endl;
	cout << "                            Definition of multiple endpoints is possible by" << endl;
	cout << "                            Repeating the argument" << endl;
	cout << "        --tcp REMOTE:PORT   Set endpoint for TCP data sent" << endl;
	cout << "   -H   --http PORT         Start webserver on the given port" << endl;
	cout << "        --probe             Probe all supported sensors" << endl;
	cout << "        --display           Pretty print sensor readings (disabled when verbose is on)" << endl;
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

static inline void unregister_child(const pid_t pid) {
	children.erase(std::remove(children.begin(), children.end(), pid), children.end());
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
			break;
		case SIGCHLD:		// Child ended
			{
				pid_t pid;		// pid of the child that exited
				int status;		// Exit status
				while( (pid = waitpid((pid_t)-1, &status, WNOHANG)) > 0) {
					cerr << "WARNING: Child " << pid << " terminated with status " << status << endl;
					unregister_child(pid);
				}
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
	
	// Mosquitto
	temp = conf.get("mosquitto_remote");
	if(temp.size() > 0) {
		MeteoMosquitto *mosquitto = NULL;
		try {
			string remote = temp;
			int port = conf.getInt("mosquitto_port", 1883);
			string username = conf.get("mosquitto_username");
			string password = conf.get("mosquitto_password");
			string topic = conf.get("mosquitto_topic");
		
			// Try to connect
			mosquitto = new MeteoMosquitto();
			if(topic.size() > 0)
				mosquitto->topic = topic;
			mosquitto->mosquitto.connect(remote.c_str(), port);
			if(username.size() > 0 && password.size() > 0)
				mosquitto->mosquitto.set_username_password(username.c_str(), password.c_str());
			mosquitto->mosquitto.loopStart();
			
			mosquittos.push_back(mosquitto);
			cout << "Mosquitto setup for " << remote << ":" << port << "." << endl;
		} catch (const char* msg) {
			if(mosquitto != NULL) delete mosquitto;
			cerr << "Error setting up mosquitto: " << msg << endl;
			exit(EXIT_FAILURE);
		}
	}
	
	// I2C device
	temp = conf.get("i2c");
	if(temp.size() > 0) {
		config.i2c_device = temp;
	}
}


static int createListeningSocket4(const int port) {
	const int sock = ::socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
		throw strerror(errno);
	
	try {
		// Setup socket
		struct sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons (port);
	
		// Set reuse address and port
		int enable = 1;
		if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
			throw strerror(errno);
		#if 0
		if (::setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int)) < 0)
			throw strerror(errno);
		#endif
		
		if (::bind(sock, (struct sockaddr *) &address, sizeof(address)) != 0)
			throw strerror(errno);
		
		// Listen
		if( ::listen(sock, 10) != 0)
			throw strerror(errno);
	} catch(...) {
		::close(sock);
		throw;
	}
	return sock;
}

/** Do the http request. return 0 on success and a negative number on error*/
static int doHttpRequest(const int fd) {
	// Process http request
	Socket socket(fd);
	
	// Read request
	String line;
	vector<String> header;
	while(true) {
		line = socket.readLine();
		if(line.size() == 0) break;
		else
			header.push_back(String(line));
	}
	
	// Search request for URL
	String url;
	String protocol = "";
	for(vector<String>::const_iterator it = header.begin(); it != header.end(); ++it) {
		String line = *it;
		vector<String> split = line.split(" ");
		if(split[0].equalsIgnoreCase("GET")) {
			if(split.size() < 2) 
				url = "/";
			else {
				url = split[1];
				if(split.size() > 2)
					protocol = split[2];
			}
		}
	}
	
	// Now process request
	if(!protocol.equalsIgnoreCase("HTTP/1.1")) {
		socket << "Unsupported protocol";
		socket.close();
		return -1;
	}
	
	// Switch requests uris
	if(url == "/" || url == "index.html" || url == "index.html") {
		// Default page
        socket.writeHttpHeader();
        socket << "<html><head><title>meteo Sensor node</title></head>";
        socket << "<body>";
        socket << "<h1>Meteo Sensor Node</h1>\n";
        
        socket << "<ul>";
#if SENSOR_BMP180 == 1
        if (config.enabled_BMP180)
            socket << "<li><a href=\"bmp180\">BMP 180 Pressure/temperature sensor</a></li>";
#endif
#if SENSOR_HTU21DF == 1
        if (config.enabled_HTU21DF)
            socket << "<li><a href=\"htu21df\">HTU21-df Humidity/temperature sensor</a></li>";
#endif
#if SENSOR_MCP9808 == 1
        if (config.enabled_MCP9808)
            socket << "<li><a href=\"mcp9808\">MCP9808 high accuracy temperature sensor</a></li>";
#endif
#if SENSOR_TSL2561 == 1
        if (config.enabled_TSL2561)
            socket << "<li><a href=\"tsl2561\">TSL2561 luminosity sensor</a></li>";
#endif
        socket << "</body>";

	} else if(url == "/bmp180") {
#if SENSOR_BMP180 == 1
        if (config.enabled_BMP180) {
            BMP180 bmp180(config.i2c_device);
	        const int ret = readSensor(bmp180);
	        if(ret != 0) {
	            socket.writeHttpHeader(500);
	            socket << "Error reading sensor";
		    	return 500;
	        } else {
	            socket.writeHttpHeader();
		        socket << bmp180.temperature() << " deg C\n";
		        socket << bmp180.pressure() << " hPa";
	        }
		} else {
		    socket.writeHttpHeader(400);
		    socket << "Sensor disabled";
		    return 400;
		}
#endif
	} else if(url == "/htu21df" || url == "/htu-21df") {
#if SENSOR_HTU21DF == 1
		if (config.enabled_BMP180) {
            HTU21DF htu21df(config.i2c_device);
	        const int ret = readSensor(htu21df);
	        if(ret != 0) {
	            socket.writeHttpHeader(500);
	            socket << "Error reading sensor";
		    	return 500;
	        } else {
	            socket.writeHttpHeader();
		        socket << htu21df.temperature() << " deg C\n";
		        socket << htu21df.humidity() << " % rel";
	        }
		} else {
		    socket.writeHttpHeader(400);
		    socket << "Sensor disabled";
		    return 400;
		}
#endif
	} else if(url == "/mcp9808") {
#if SENSOR_MCP9808 == 1
		if (config.enabled_BMP180) {
            MCP9808 mcp9808(config.i2c_device);
	        const int ret = readSensor(mcp9808);
	        if(ret != 0) {
	            socket.writeHttpHeader(500);
	            socket << "Error reading sensor";
		    	return 500;
	        } else {
	            socket.writeHttpHeader();
		        socket << mcp9808.temperature() << " deg C\n";
	        }
		} else {
		    socket.writeHttpHeader(400);
		    socket << "Sensor disabled";
		    return 400;
		}
#endif
	} else if(url == "/tsl2561") {
#if SENSOR_TSL2561 == 1
		if (config.enabled_BMP180) {
            TSL2561 tsl2561(config.i2c_device);
	        const int ret = readSensor(tsl2561);
	        if(ret != 0) {
	            socket.writeHttpHeader(500);
	            socket << "Error reading sensor";
		    	return 500;
	        } else {
	            socket.writeHttpHeader();
		        socket << tsl2561.visible() << " visible\n";
		        socket << tsl2561.ir() << " ir";
	        }
		} else {
		    socket.writeHttpHeader(400);
		    socket << "Sensor disabled";
		    return 400;
		}
#endif
	} else if(url == "/sensors" || url == "/readings") {
		
    	// Setting up sensors
		int errors = 0;
		stringstream ss;
#if SENSOR_BMP180 == 1
		BMP180 bmp180(config.i2c_device);
		if (config.enabled_BMP180) {
			if(readSensor(bmp180) == 0) {
				ss << "BMP180: " << bmp180.temperature() << " deg C, " << bmp180.pressure() << " hPa" << "\r\n";
			} else
				errors++;
		}
#endif
#if SENSOR_HTU21DF == 1
		HTU21DF htu21df(config.i2c_device);
		if (config.enabled_HTU21DF) {
			if(readSensor(htu21df) == 0) {
				ss << "HTU21D-F: " << htu21df.temperature() << " deg C, " << htu21df.humidity() << " % rel" << "\r\n";
			} else
				errors++;
		}
#endif
#if SENSOR_MCP9808 == 1
		MCP9808 mcp9808(config.i2c_device);
		if (config.enabled_MCP9808) {
			if(readSensor(mcp9808) == 0) {
				ss << "MCP9808: " << mcp9808.temperature() << " deg C" << "\r\n";
			} else
				errors++;
		}
#endif
#if SENSOR_TSL2561 == 1
		TSL2561 tsl2561(config.i2c_device);
		if (config.enabled_TSL2561) {
			if(readSensor(tsl2561) == 0) {
				ss << "TSL2561: " << tsl2561.visible() << " visible, " << tsl2561.ir() << " ir" << "\r\n";
			} else
				errors++;
		
		}
#endif

		// Manual header
		
		int statusCode = 200;
		if(errors > 0) statusCode = 500;
		
        socket.print("HTTP/1.1 ");
        socket.print(::to_string(statusCode));
        socket.print(" OK\r\n");
        socket.print("Content-Type:text/plain\r\n");
        socket.print("\r\n");
		
		socket << ss.str();
		socket.close();
	} else {
		// Not found
		socket << "HTTP/1.1 404 Not Found\n";
		socket << "Content-Type: text/html\n\n";
		socket << "<html><head><title>Not found</title></head><body><h1>Not found</h1>";
		socket << "<p>Error 404 - Sensor or page not found. Maybe you want to <a href=\"index.html\">go back to the homepage</a></p>";
		return 400;
	}
	
	// All good
	return 0;
}


static pid_t startWebserver(const int port) {
	pid_t pid = fork();
	if(pid < 0) 
		throw strerror(errno);
	if(pid > 0) {
		// Parent process. return pid
		return pid;
	} else {
		try {
			// Child process. Actually start webserver
			const int sock = createListeningSocket4(port);
		
			while(true) {
			    const int fd = ::accept(sock, NULL, 0);
			    // Fork child
			    pid = fork();
			    if(pid < 0) {
			    	cerr << "Cannot fork in web server: " << strerror(errno) << endl;
			    	::close(fd);
			    	continue;
			    }
			    if(pid == 0) {
			    	::close(sock);
			    	
			    	// Process request
			    	const int ret = doHttpRequest(fd);
			    	::close(fd);
			    	exit(ret);
			    	return -1;		// Should never occur
			    } else {
			    	// Parent process. We don't need the socket here
			    	::close(fd);
			    	
			    	// Wait for the child to finish
			    	int status;
			    	waitpid(pid, &status, 0);
			    	// XXX Don't care about anything that could go wrong ...
			    	
			    	continue;
			    }
			}
	    } catch (const char* msg) {
	    	cerr << "Webserver error: " << msg << endl;
	    	exit(EXIT_FAILURE);
	    } catch (...) {
	    	cerr << "Uncaught webserver error" << endl;
	    	exit(EXIT_FAILURE);
	    }
	    return pid;
	}
}


void cleanup() {
	// Close all children
	for(vector<pid_t>::const_iterator it = children.begin(); it != children.end(); ++it) {
		const pid_t pid = *it;
		kill(pid, SIGTERM);
	}
	for(vector<MeteoMosquitto*>::iterator it = mosquittos.begin(); it != mosquittos.end(); ++it)
		delete (*it);
	mosquittos.clear();
	children.clear();
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
