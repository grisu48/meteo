/* =============================================================================
 * 
 * Title:         meteo Server
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Reception, collection and storage of meteo data
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <fstream>
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


static void recvCallback(std::string, std::map<std::string, double>);
static void sig_handler(int signo);
long getSystemTime(void);
static void sleep(long millis, long micros);
static int pthread_terminate(pthread_t tid);
static void printHelp(const char* progname);
static void cleanup(void);
/** Setup serial device for reading. Returns thread id */
static pthread_t  setup_serial(const char* dev);
/** Rung program as deamon */
static void deamonize();

class Instance {
protected:
	/** Meteo nodes */
	map<string, Node> _nodes;
	
	/** Udp receivers */
	vector<UdpReceiver*> udpReceivers;
	
	/** TCP broadcast servers */
	vector<TcpBroadcastServer*> tcpBroadcasts;
	
	/** Timer thread for checking thread alive */
	pthread_t tid_alive = 0;
	
	/** Database instance, if set */
	MySQL *db = NULL;
	
	/** File where the current readings are written to */
	string outFile;
	
	
	/** Delay when to write to database in milliseconds. Default : 2 Minutes*/
	long writeDBDelay = 2L * 60L * 1000L;
	
	/** Period interval in milliseconds */
	long period_ms = 1000L;
	
	/** Milliseconds that is considered as ALIVE.
	  * If a node doesn't respond within this amount of time, then it's considered dead
      */
	long alive_period = 60L * 1000L;
	
	volatile bool running = true;
	
	/** Threads */
	vector<pthread_t> threads;
public:
	Instance();
	virtual ~Instance();
	
	void runAliveThread(void);
	
	/** Create new Udp receiver on the given port.
	  * Creates and adds the given receiver. The receiver is NOT started
	  @returns created instance
	  */
	UdpReceiver* createUdpReceiver(const int port)  {
		UdpReceiver *recv = new UdpReceiver(port);
		recv->setReceiveCallback(recvCallback);
		udpReceivers.push_back(recv);
		return recv;
	}
	
	/** Assign database */
	void setDatabase(MySQL *db) { this->db = db; }
	
	void setOutputFile(string filename) {
		this->outFile = filename;
	}
	
	bool addTcpBroadcastServer(const int port) {
	    try {
    	    TcpBroadcastServer *server = new TcpBroadcastServer(port);
    	    server->start();
    	    tcpBroadcasts.push_back(server);
	        return true;
	    } catch (const char* msg) {
	        cerr << "Error setting up tcp broadcast server on port " << port << ": " << msg << endl;
	        cerr << strerror(errno) << endl;
	        return false;
	    }
	}
	
	bool isRunning(void) const { return this->running; }
	
	void addThread(pthread_t tid) { this->threads.push_back(tid); }
	
	/** Set database write delay in milliseconds. Ignored if < 0 */
	void setDBWriteDelay(long delay) {
	    if(delay < 0) return;
	    else
	        this->writeDBDelay = delay;
	}
	
	/** Receive data from a node */
	void receive(string name, map<string, double> values) {
		if(name.size() == 0) return;
		if(values.size() == 0) return;
		
		// Insert node if not yet existing
		if (_nodes.find(name) == _nodes.end()) {
			Node node(name);
			_nodes[name] = node;
			cout << "APPEARED ";
		} else 
			cout << "RECEIVED ";

		Node& node = _nodes[name];
		node.setTimestamp(getSystemTime());
		
		cout << node.name();
		for(map<string, double>::iterator it = values.begin(); it != values.end(); it++) {
			node.pushData(it->first, it->second);
			cout << " " << it->first << " = " << it->second;
		}
		cout << endl;
		
		// Broadcast node
		if(tcpBroadcasts.size() > 0) {
		    string xml = node.xml() + "\n";
		    for(vector<TcpBroadcastServer*>::iterator it = tcpBroadcasts.begin(); it != tcpBroadcasts.end(); ++it) {
		        (*it)->broadcast(xml);
		    }
		}
	}

	vector<Node> nodes(void) {
		vector<Node> res;
		for(map<string, Node>::iterator it = _nodes.begin(); it != _nodes.end(); it++) {
			res.push_back(it->second);
		}
		return res;
	}
	
	
	/** Close everything */
	void close(void) {
		this->running = false;
		// Close udp receivers
		vector<UdpReceiver*> udpReceivers(this->udpReceivers);
		this->udpReceivers.clear();
		for(vector<UdpReceiver*>::iterator it = udpReceivers.begin(); it != udpReceivers.end(); ++it)
			delete *it;
		
		// Close tcp broadcasts
		vector<TcpBroadcastServer*> tcpBroadcasts(this->tcpBroadcasts);
		this->tcpBroadcasts.clear();
		for(vector<TcpBroadcastServer*>::iterator it = tcpBroadcasts.begin(); it != tcpBroadcasts.end(); ++it) {
		    delete *it;
		}
		
		
		// Close alive thread
		if(this->tid_alive > 0) {
			pthread_t tid = this->tid_alive;
			this->tid_alive = 0;
			pthread_terminate(tid);
		}
		
		// Close other threads
		for(vector<pthread_t>::iterator it = threads.begin(); it != threads.end(); ++it) {
		    pthread_terminate(*it);
		}
		threads.clear();
	}
	
	/** Clean all nodes that are not marked alive. Marks all alive nodes as not alive
	  * @returns Number of elements deleted
	  */
	int tidyDead(void) {
	#if 0
		map<string, Node>::iterator it = this->_nodes.begin();
		int counter = 0;
		while(it != this->_nodes.end()) {
			const long timestamp = getSystemTime();
			const long tolerance = timestamp - this->alive_period;
			Node &node = it->second;
			if(node.timestamp() < tolerance) {
				cout << "Node[" << node.name() << "] dead. Inactive for ";
				cout << (timestamp - node.timestamp()) << " ms" << endl;
				it = this->_nodes.erase(it);
				counter++;
				continue;
			} else {
				++it;
			}
		}
		return counter;
	#endif
	    return 0;
	}
	
	void writeToDB(void);
};

/** Singleton instance*/
static Instance instance;





/* ==== MAIN ENTRANCE POINT ================================================= */

struct {
    bool enabled = false;
    string hostname;
    string username;
    string password;
    string database;
    int port = 3306;
} typedef db_t;

int main(int argc, char** argv) {
    bool daemon = false;
	{
		vector<int> tcp_ports;
		vector<int> udp_ports;
		vector<int> tcp_broadcasts;
		vector<string> serials;
		db_t db;
		
		// Parse program arguments
		try {
			for(int i=1;i<argc;i++) {
				string arg = string(argv[i]);
				const bool isLast = i >= argc-1;
				if(arg.length() == 0) continue;
				else if(arg.at(0) == '-') {
				
					// Switch program arguments
					if(arg == "--udp") {
						if(isLast) throw "Missing argument: udp port";
						udp_ports.push_back(atoi(argv[++i]));
					} else if(arg == "--tcp") {
						if(isLast) throw "Missing argument: tcp port";
						tcp_broadcasts.push_back(atoi(argv[++i]));
					} else if(arg == "--mysql" || arg == "--db") {
						// "user:password@hostname/database"
						arg = String(argv[++i]);
						size_t index = arg.find(":");
						if(index == string::npos) throw "Illegal database format";
						String dbUsername = arg.substr(0, index);
						arg = arg.substr(index+1);
						index = arg.find("@");
						if(index == string::npos) throw "Illegal database format";
						String dbPassword = arg.substr(0, index);
						arg = arg.substr(index+1);
						index = arg.find("/");
						if(index == string::npos) throw "Illegal database format";
						String dbHostname = arg.substr(0, index);
						String dbDatabase = arg.substr(index+1);

						if(dbHostname.isEmpty()) throw "Missing database hostname";
						if(dbDatabase.isEmpty()) throw "Missing database name";
						if(dbUsername.isEmpty()) throw "Missing database username";
						
						db.enabled = true;
						db.hostname = dbHostname;
						db.database = dbDatabase;
						db.username = dbUsername;
						db.password = dbPassword;
						
					} else if(arg == "--db-write-delay") {
					    if(isLast) throw "Missing argument: Database write delay";
					    // Write delay in milliseconds
					    long delay = atol(argv[++i]);
					    if(delay < 0) throw "Illegal argument: Database write delay must be positive";
					    instance.setDBWriteDelay(delay);
					    cout << "Set database write delay to " << delay << " ms" << endl;
					    
				    } else if(arg == "--serial") {
				        if(isLast) throw "Missing argument: Serial device file";
				        serials.push_back(string(argv[++i]));
				        
				    } else if(arg == "-d" || arg == "--daemon") {
				        daemon = true;
					    
					} else if(arg == "--help") {
						printHelp(argv[0]);
						return EXIT_SUCCESS;
					} else if(arg == "-f") {
						if(isLast) throw "Missing argument: Filename";
						instance.setOutputFile(string(argv[++i]));
					} else {
						cerr << "Illegal argument: " << arg << endl;
						return EXIT_FAILURE;
					}
				
				} else {
					cerr << "Illegal program argument: " << arg << endl;
					return EXIT_FAILURE;
				}
			}
		} catch (const char* msg) {
			cerr << msg << endl;
			return EXIT_FAILURE;
		}
		
		
		
		if(daemon) {
            deamonize();
            cout << "Running as daemon now" << endl;
        }
		
		// Setup udp ports first
		if(udp_ports.size() > 0) {
			cout << "Setting up " << udp_ports.size() << " udp listener(s):" << endl;
			for(vector<int>::iterator it = udp_ports.begin(); it != udp_ports.end(); it++) {
				const int port = *it;
				cout << "\t" << port << " ... ";
				
				try {
					UdpReceiver *recv = instance.createUdpReceiver(port);
					recv->start();
					cout << "ok" << endl;
				} catch (const char *msg) {
					cout << "failed: " << msg << "." << endl; cout.flush();
					cerr << "Socketerror: " << strerror(errno) << endl; cerr.flush();
					exit(EXIT_FAILURE);
				}
				
				
			}
		}
		
		// Setup tcp broadcasts
		if(tcp_broadcasts.size() > 0) {
		    cout << "Setting up " << tcp_broadcasts.size() << " tcp broadcast(s):" << endl;
			for(vector<int>::iterator it = tcp_broadcasts.begin(); it != tcp_broadcasts.end(); it++) {
			    const int port = *it;
			    cout << "  Port " << port << " ... "; cout.flush();
			    if(instance.addTcpBroadcastServer(port))
			        cout << "ok" << endl;
			    else {
			        cout << "failed" << endl;
			        exit(EXIT_FAILURE);
			    }
			}
		}
		
		// Setup serial ports
		for(vector<string>::iterator it = serials.begin(); it != serials.end(); ++it) {
    		string serial = *it;
	        try {
	            pthread_t tid = setup_serial(serial.c_str());
	            instance.addThread(tid);
	            
	        } catch (const char* msg) {
	            cerr << "Warning: Error setting up serial: " << msg << endl;
	            // return EXIT_FAILURE;
	        }
		}
		
		if(db.enabled) {
			try {
				MySQL *mysql= new MySQL(db.hostname, db.username, db.password, db.database);
				mysql->connect();
    			cout << "Database: " << mysql->getDBMSVersion() << endl;
				mysql->close();
				
				instance.setDatabase(mysql);
			} catch (const char* msg) {
				cerr << "Error connecting to database: " << msg << endl;
				return EXIT_FAILURE;
			}
		}
	}
	
	// Register signals
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	atexit(cleanup);
	
	
	// Read input
	if(daemon) {
	    // Wait until the end of time
	    while(instance.isRunning()) 
	        sleep(3600);
	} else {
	    string line;
	    Parser parser;
	    cout << "Server setup completed." << endl;
	    while(true) {
		    // cout << "> "; cout.flush();
		    if(!getline(cin, line)) break;
		    if(line.size() == 0) continue;
		
		    try {
			    if (line == "exit" || line == "quit") break;
			    else if(line == "list") {
				    // List all nodes
				    vector<Node> nodes = instance.nodes();
				    if (nodes.size() == 0) {
					    cout << "No nodes present" << endl;
				    } else {
					    for(vector<Node>::iterator it = nodes.begin(); it != nodes.end(); it++) {
						    cout << "Node[" << (*it).name() << "]";
						    map<string, double> values = (*it).values();
						    if(values.size() == 0) {
							    cout << " Empty";
						    } else {
							    for(map<string,double>::iterator jt = values.begin(); jt != values.end(); jt++)
								    cout << " " << jt->first << " = " << jt->second;
						    }
						    cout << endl;
					    }
				    }
			    } else if(line == "help") {
				    cout << "METEO server instance" << endl;
				    cout << "Use 'exit' or 'quit' to leave or input a test packet." << endl;
			    } else {
				    // Try to parse
				    if(parser.parse(line)) {
					    recvCallback(parser.node(), parser.values());
					    parser.clear();
				    } else {
					    cerr << "Input parse failed" << endl; cerr.flush();
				    }
			    }
		    } catch(const char* msg) {
			    cout.flush();
			    cerr << "Error: " << msg << endl;
			    cerr.flush();
		    }
		
	    }
	    cout << "Exiting instance." << endl;
	}
    return EXIT_SUCCESS;
}


static void alive_thread(void* arg) {
	(void)arg;
	
	// Thread is cancellable immediately
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	instance.runAliveThread();
}

Instance::Instance() {
	this->tid_alive = 0;
	
	int ret;
	ret = pthread_create(&this->tid_alive, NULL, (void* (*)(void*))alive_thread, NULL);
	if(ret < 0) {
		cerr << "Error creating alive thread" << endl;
	}
}

Instance::~Instance() {
	this->close();
	if(this->db != NULL) delete this->db;
}


void Instance::runAliveThread(void) {
	
	long lastWriteTimestamp = getSystemTime();		// Timestamp when the last write occurred
	
	while(this->tid_alive > 0) {
		try {
			const long sysTime = getSystemTime();
			long sleeptime = sysTime;
			int counter = this->tidyDead();
			if(counter > 0) {
				cout << "Cleanup: " << counter << " nodes" << endl;
			}
			
			// Check if we need to write the stuff to the database
			
			if (sysTime - lastWriteTimestamp > writeDBDelay) {
			    long writeDBDelay = -getSystemTime();
				this->writeToDB();
				lastWriteTimestamp = sysTime;
				writeDBDelay += getSystemTime();
				cout << "Data written to database (" << writeDBDelay << " ms)" << endl;
			}
			
			
			
			sleeptime -= getSystemTime() - period_ms;
		
			if(sleeptime > 0) {
				// cout << sleeptime << " ms" << endl;
				sleep(sleeptime, 0);
			}
		} catch (const char* msg) {
			cout.flush();
			cerr << "Error: " << msg << endl;
			cerr.flush();
		}
	}
}

void Instance::writeToDB(void) {
	// Write to file
	if(this->outFile.size() > 0) {
		ofstream fout(this->outFile);
		if(fout.is_open()) {
			
			fout << "# Name, Timestamp, [NAME=VALUE]" << endl;
			
			// Write all nodes
			for(map<string, Node>::iterator it= this->_nodes.begin(); it != this->_nodes.end(); ++it) {
				string key = it->first;
				Node &node = it->second;
				
				
				map<string, double> values = node.values();
				string name = node.name();
				fout << name << "," << node.timestamp();
				for(map<string, double>::iterator jt = values.begin(); jt != values.end(); jt++)
					fout << "; " << jt->first << " = " << jt->second;
				fout << endl;
			}
			
			fout.close();
		}
	}

	if(this->db != NULL && this->_nodes.size() > 0) {
		this->db->connect();
		long errors = 0;
		
		const long t_max = getSystemTime() - alive_period;
		
		// Write all nodes, that are fresh
		for(map<string, Node>::iterator it= this->_nodes.begin(); it != this->_nodes.end(); ++it) {
			string key = it->first;
			Node &node = it->second;
			
			long timestamp = node.timestamp();
			if(timestamp > t_max) {
			    
			    try {
				    this->db->push(node);
			    } catch (const char* msg) {
				    cerr << "Error writing node " << key << " to database: " << msg << endl;
				    errors++;
			    }
			
			}
		}
		
		if(errors > 0) {
			cerr << errors << " error(s) occurred while writing to database" << endl;
		}
		
		this->db->close();
	}
	
	
}

static void recvCallback(std::string node, std::map<std::string, double> values) {
//	cout << "Recevied: " << node << " with " << values.size() << " values" << endl;
	instance.receive(node, values);
}

static void sig_handler(int signo) {
	switch(signo) {
		case SIGINT:
		case SIGTERM:
			cerr << "Terminating ... " << endl;
			instance.close();
			exit(EXIT_FAILURE);
			break;
	}
}

long getSystemTime(void) {
	struct timeval tv;
	int64_t result;
	gettimeofday(&tv, 0);
	result = tv.tv_sec;
	result *= 1000L;
	result += (int64_t)tv.tv_usec/1000L;
	return result;
}

static void sleep(long millis, long micros) {
	struct timespec tt, rem;
	int ret;

	tt.tv_sec = millis/1000L;
	millis -= tt.tv_sec*1000L;
	tt.tv_nsec = (micros + millis*1000L)*1000L;
	ret = nanosleep(&tt, &rem);
	if(ret < 0) {
		switch(errno) {
		case EFAULT:
			throw "Error setting struct";
		case EINTR:
			// Note: In "rem" the remaining time is stored
			throw "Interrupted";
		case EINVAL:
			throw "Illegal value";
		}
	}
}

static int pthread_terminate(pthread_t tid) {
	if(tid == 0) return -1;
	int ret;
	
	ret = pthread_cancel(tid);
	if(ret < 0) {
		// Kill thread
		ret = pthread_kill(tid, SIGTERM);
		if(ret < 0) return ret;
	}
	pthread_join(tid, NULL);
	return ret;
	
}

static void printHelp(const char* progname) {
	// user:password@hostname/database
	
	cout << "Meteo Server Instance" << endl;
	cout << "  2016, Felix Niederwanger <felix@feldspaten.org>" << endl;
	cout << endl;
	
	cout << "Usage: " << progname << " [OPTIONS]" << endl;
	cout << "OPTIONS" << endl;
	cout << "  -h   --help                      Print this help message" << endl;
	cout << "  --udp PORT                       Listen on PORT for udp messages. Multiple definitions allowed" << endl;
	cout << "  --tcp PORT                       Listen on PORT for tcp broadcast clients. Multiple definitions allowed" << endl;
	cout << "  --mysql REMOTE                   Set REMOTE as MySQL database instance." << endl;
	cout << "          REMOTE is in the form: user:password@hostname/database" << endl;
	cout << "  --db-write-delay MILLIS          Set the delay for database writes in milliseconds" << endl;
	cout << "  -f FILE                          Periodically write the current settings to FILE" << endl;
	cout << "  --serial FILE                    Open FILE as serial device file and read contents from it" << endl;
	cout << "  -d  --daemon                     Run as daemon" << endl;
	cout << endl;
}

static void cleanup(void) {
	MySQL::Finalize();
}

/** Thread for setting up serial */
static void* serial_thread(void* arg) {
    const char* dev = (char*)arg;
    
    
	// Thread is cancellable immediately
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    
    
    try {
        Serial serial(dev);
        Parser parser;
        cout << "Serial reader at device " << dev << " started" << endl;
        while(!serial.eof()) {
            string line = serial.readLine();
            if(line.size() == 0 || line.at(0) == '#') continue;
            
            // cout << "# [" << dev << "] " << line << endl;
            
            // Parse line
		    if(parser.parse(line)) {
			    recvCallback(parser.node(), parser.values());
			    parser.clear();
		    }
        }
    } catch (const char* msg) {
        cerr << "Error in Serial " << dev << ": " << msg << endl;
        cerr << "Reading failure from serial " << dev << ". Closing instance" << endl;
    }
    
    return NULL;
}

static pthread_t setup_serial(const char* dev) {
    // Create thread
    pthread_t tid;
    
	int ret;
	ret = pthread_create(&tid, NULL, serial_thread, (void*)dev);
	if(ret < 0) throw "Error creating serial thread";
	if(pthread_detach(tid) != 0) throw "Error detaching serial thread";
	return tid;
}

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
