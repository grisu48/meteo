/*
 * main.cpp
 *
 *  Created on: 15 Sep 2016
 *      Author: phoenix
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <mysql.h>

#include "serial.hpp"
#include "string.hpp"

using namespace std;
using io::Serial;
using flex::String;

#define VERSION "0.1"
#define DEVICE "/dev/ttyUSB0"

class Packet {
private:
	/** ID of the station */
	int _id;
	/** Light (value from 0 .. 255) */
	float _light;
	/** Humditiy in rel. %*/
	float _humidity;
	/** Temperature in degree celcius*/
	float _temperature;
	/** Battery status - 0 = good, 1 = low voltage */
	int _battery;

public:
	Packet(int id, float light, float humidity, float temperature, int battery) {
		this->_id = id;
		this->_light = light;
		this->_humidity = humidity;
		this->_temperature = temperature;
		this->_battery = battery;
	}

	int stationId(void) const { return this->_id; }
	float light(void) const { return this->_light; }
	float lightPercent(void) const { return this->_light/2.55; }
	float humidity(void) const { return this->_humidity; }
	float temperature(void) const { return this->_temperature; }
	int battery(void) const { return this->_battery; }
	bool isBatteryOk(void) const { return this->_battery == 0; }

	string toString(void) const {
		stringstream ss;

		ss << "Room[" << stationId() << "] -- " << lightPercent() << "% ligth; " << humidity() << " %. rel humidity at ";
		ss << temperature() << " degree Celcius";
		if (!isBatteryOk()) ss << "  -- Low battery!" << endl;

		return ss.str();
	}
};

class MySQL {
private:
	MYSQL *conn = NULL;

public:
	MySQL(const char* hostname, const char* database, const char* username, const char* password, int port = 3306) {
		conn = mysql_init(NULL);
		if(conn == NULL) throw "Insufficient memory";

		if(!mysql_real_connect(conn, hostname, username, password, database, port, NULL, 0))
			throw "Cannot connect to server";
	}

	MySQL(string hostname, string database, string username, string password, int port = 3306) :
		MySQL(hostname.c_str(), database.c_str(), username.c_str(), password.c_str(), port) {}

	/**
	 * Execute sql. Return 0 on success
	 * @param sql to be executed
	 * @return 0 on success, another value on error
	 */
	int execute(const char* sql, size_t len) {
		if(this->conn == NULL) throw "Connection closed";

		int ret = mysql_real_query(conn, sql, len);
		return ret;
	}

	/**
	 * Execute sql. Return 0 on success
	 * @param sql to be executed
	 * @return 0 on success, another value on error
	 */
	int execute(const string &str) {
		return execute(str.c_str(), str.length());
	}

	string escapeString(const char* str, size_t len) const {
		if(this->conn == NULL) throw "Connection closed";

		char* dst = new char[len*2];
		memset(dst, '\0', sizeof(char)*len*2);

		ssize_t size = mysql_real_escape_string(conn, dst, str, len);

		if(size < 0) {
			delete[] dst;
			return "";
		}

		string result = string(dst, size);
		delete[] dst;
		return result;
	}

	virtual ~MySQL() {
		this->close();

	}

	void close(void) {
		if(conn != NULL)
			mysql_close(conn);
		conn = NULL;
	}

	string getDBMSVersion(void) {
		if(this->conn == NULL) throw "Connection closed";

		if(execute("SELECT VERSION();")) throw "Error executing sql";

		string version = "";

		MYSQL_RES *res = mysql_use_result(conn);
		if(res == NULL) throw "Error getting result structure";
		MYSQL_ROW row;

		// Iterate over rows
		while ((row = mysql_fetch_row(res)) != NULL) {
			version = string(row[0]);
			break;
		}

		mysql_free_result(res);

		return version;
	}

	void createStationTable(const int stationId) {
		if(this->conn == NULL) throw "Connection closed";
		stringstream ss;
		ss << "CREATE TABLE IF NOT EXISTS `meteo`.`station_" << stationId << "` ( `timestamp` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP , `light` FLOAT NOT NULL , `humidity` FLOAT NOT NULL , `temperature` FLOAT NOT NULL , PRIMARY KEY (`timestamp`)) ENGINE = InnoDB CHARSET=utf8 COLLATE utf8_general_ci COMMENT = 'Station " << stationId << " table';";
		string sql = ss.str();
		if(execute(sql) != 0) throw "Error creating table";
	}

	void insertPacket(const Packet &packet) {
		if(this->conn == NULL) throw "Connection closed";
		this->createStationTable(packet.stationId());
		stringstream ss;
		ss << "INSERT INTO `station_" << packet.stationId() << "` (`timestamp`, `light`, `humidity`, `temperature`) ";
		ss << "VALUES (CURRENT_TIMESTAMP, '" << packet.light() << "', '" << packet.humidity() << "', '" << packet.temperature() << "');";
		string sql = ss.str();
		if(execute(sql) != 0) throw "Error inserting packet";
	}

	unsigned int errono(void) const {
		if(this->conn == NULL) return 0;
		return mysql_errno(this->conn);
	}

	const char* error_str(void) const {
		if(this->conn == NULL) return 0;
		return mysql_error(this->conn);
	}
};

static void packetReceived(const Packet &packet);
static void cleanup(void);
static void printHelp(const char* progname);
static void sig_handler(int sig_no);

static MySQL *mysql;
static volatile bool running = true;



int main(int argc, char** argv) {
	string device = DEVICE;
	bool useDatabase = false;
	String dbHostname = "localhost";
	String dbDatabase = "meteo";
	String dbUsername = "meteo";
	String dbPassword = "";

	try {
		for(int i=1;i<argc;i++) {
			String arg = String(argv[i]);
			if(arg.isEmpty()) continue;
			const bool isLast = i >= argc-1;
			if(arg.at(0) == '-') {
				// Program argument
				if(arg == "-h" || arg == "--help") {
					printHelp(argv[0]);
					return EXIT_SUCCESS;
				} else if(arg == "-d" || arg == "--device") {
					if(isLast) throw "Missing argument: device";
					device = argv[++i];
				} else if(arg == "--db" || arg == "--mysql" || arg == "--database") {
					if(isLast) throw "Missing argument: database";
					// "user:password@hostname/database"
					arg = String(argv[++i]);
					size_t index = arg.find(":");
					if(index == string::npos) throw "Illegal database format";
					dbUsername = arg.substr(0, index);
					arg = arg.substr(index+1);
					index = arg.find("@");
					if(index == string::npos) throw "Illegal database format";
					dbPassword = arg.substr(0, index);
					arg = arg.substr(index+1);
					index = arg.find("/");
					if(index == string::npos) throw "Illegal database format";
					dbHostname = arg.substr(0, index);
					dbDatabase = arg.substr(index+1);

					if(dbHostname.isEmpty()) throw "Missing database hostname";
					if(dbDatabase.isEmpty()) throw "Missing database name";
					if(dbUsername.isEmpty()) throw "Missing database username";
					useDatabase = true;
				}
			} else {
				cerr << "Illegal argument " << i << ": " << arg << endl;
				return EXIT_FAILURE;
			}
		}
	} catch (const char* msg) {
		cerr << msg << endl;
		cerr.flush();
		cout << "If you need help, type " << argv[0] << " --help" << endl;
		return EXIT_FAILURE;
	}

	cout << "meteoLink -- 2016, Felix Niederwanger" << endl;
	cout << "  Version " << VERSION << endl;

	atexit(cleanup);
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);


	// Connect to server
	if(useDatabase) {
		try {
			mysql = new MySQL(dbHostname, dbDatabase, dbUsername, dbPassword);
			cout << "Connected to database: " << mysql->getDBMSVersion() << endl;
		} catch (const char* msg) {
			cerr << "Error setting up database connection: " << msg << endl;
			mysql = NULL;
		}
	}

	// Setup serial
	cout << "Opening device: " << device << " ... " << endl;
	try {
		Serial serial(device);
		serial.setBaudRate(B57600);

		cout << "Device openend." << endl;

		stringstream buffer;		// Buffer for the packets
		while(running) {
			String line = String(serial.readLine()).trim();
			if (line.size() == 0) continue;

			// Check if comment line
			if(line.at(0) == '#') continue;

			// Data packet line. Parse it
			try {

				std::vector<String> splitted = line.split(" ");
				if(splitted.size() < 6) throw "Missing data";

				if(splitted[0] != "ROOM") throw "Illegal header";

				// Parse other data
				int id;		// ID of the node
				float light;			// Light (0..255)
				float humidity;			// Humidity in rel. %
				float temperature;		// Temperature in degree celcius
				int battery;			// Battery status (0 = good, 1 = low voltage)

				id = atoi(splitted[1].c_str());
				light = (float)atof(splitted[2].c_str());
				humidity = (float)atof(splitted[3].c_str());
				temperature = (float)atof(splitted[4].c_str()) * 0.1;
				battery = atoi(splitted[5].c_str());

				Packet packet(id, light, humidity, temperature, battery);
				packetReceived(packet);

			} catch (const char* msg) {
				cerr << "CORRUPT: " << msg << " :: " << line << endl;
			}
		}

	} catch (const char* msg) {
		cerr << "Serial error: " << msg << endl;
		return EXIT_FAILURE;
	}

	cout << "Bye" << endl;
	return EXIT_SUCCESS;
}



static void packetReceived(const Packet &packet) {
	try {
		if(mysql != NULL) {
			mysql->insertPacket(packet);
			cout << "Packet inserted: ";
		}
	} catch (const char* msg) {
		cerr << "Error " << mysql->errono() << " while inserting packet: " << mysql->error_str() << endl;
	}

	cout << packet.toString() << endl;
}


static void cleanup(void) {
	if(mysql != NULL) delete mysql;
}


static void sig_handler(int sig_no) {
	switch(sig_no) {
	case SIGINT:
		cerr << "SIGINT received. Terminating." << endl;
		exit(EXIT_FAILURE);
		break;
	case SIGTERM:
		cerr << "SIGTERM received. Terminating" << endl;
		exit(EXIT_FAILURE);
		break;
	}
}


static void printHelp(const char* progname) {
	cout << "meteoLink -- 2016, Felix Niederwanger" << endl;
	cout << "  Version " << VERSION << endl;
	cout << endl;

	cout << "Usage: " << progname << " [OPTIONS]" << endl;
	cout << "OPTIONS" << endl;
	cout << "  -h      --help                Print this help message" << endl;
	cout << "  -d DEV" << endl;
	cout << "  --device DEV                  Set device file to DEV. Default value: " << DEVICE << endl;
	cout << "  --db REMOTE" << endl;
	cout << "  --mysql REMOTE" << endl;
	cout << "  --database REMOTE             Set database to REMOTE. REMOTE is in the following format:" << endl;
	cout << "                                user:password@hostname/database" << endl;
}
