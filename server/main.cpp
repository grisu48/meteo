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
#include "node.hpp"
#include "parser.hpp"


// Alive check cycle in seconds
#define ALIVE_CYCLE 500
// Alive tolerance in seconds
#define ALIVE_TOLERANCE 15


using namespace std;



static void recvCallback(std::string, std::map<std::string, double>);
static void sig_handler(int signo);
long getSystemTime(void);
static void sleep(long millis, long micros);


class Instance {
protected:
	/** Meteo nodes */
	map<string, Node> _nodes;
	
	/** Udp receivers */
	vector<UdpReceiver*> udpReceivers;
	
	/** Timer thread for checking thread alive */
	pthread_t tid_alive = 0;
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
		node.setAlive();
		node.setTimestamp(getSystemTime());
		
		cout << node.name();
		for(map<string, double>::iterator it = values.begin(); it != values.end(); it++) {
			node.pushData(it->first, it->second);
			cout << " " << it->first << " = " << it->second;
		}
		cout << endl;
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
		cout << "Closing .. " << endl;
		// Close udp receivers
		vector<UdpReceiver*> udpReceivers(this->udpReceivers);
		this->udpReceivers.clear();
		for(vector<UdpReceiver*>::iterator it = udpReceivers.begin(); it != udpReceivers.end(); ++it)
			delete *it;
		
		
	}
	
	/** Clean all nodes that are not marked alive. Marks all alive nodes as not alive
	  * @returns Number of elements deleted
	  */
	int tidyDead(void) {
		map<string, Node>::iterator it = this->_nodes.begin();
		int counter = 0;
		while(it != this->_nodes.end()) {
			const long timestamp = getSystemTime();
			const long tolerance = timestamp - ALIVE_TOLERANCE * 1000L;
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
	}
	
	void writeToDB(void);
};

/** Singleton instance*/
static Instance instance;


/* ==== MAIN ENTRANCE POINT ================================================= */


int main(int argc, char** argv) {

	{
		vector<int> tcp_ports;
		vector<int> udp_ports;
		
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
					cout << "failed: " << msg << endl; cout.flush();
					cerr << strerror(errno) << endl; cerr.flush();
				}
				
				
			}
		}
	}
	
	// Register signals
	signal(SIGTERM, sig_handler);
	signal(SIGINT, sig_handler);
	
	
	// Read input
	string line;
	Parser parser;
	cout << "Server setup completed." << endl;
	while(true) {
		cout << "> "; cout.flush();
		if(!getline(cin, line)) break;
		if(line.size() == 0) continue;
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
		
		
	}
	
	
	// instance.close();			-- Will happen automatically
	exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}


static void alive_thread(void* arg) {
	(void)arg;
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
}


void Instance::runAliveThread(void) {
	while(this->tid_alive > 0) {
		long sleeptime = getSystemTime();
		int counter = this->tidyDead();
		if(counter > 0) {
			cout << "Cleanup: " << counter << " nodes" << endl;
		}
		this->writeToDB();
		sleeptime -= getSystemTime() - ALIVE_CYCLE;
		
		if(sleeptime > 0) {
			// cout << sleeptime << " ms" << endl;
			sleep(sleeptime, 0);
		}
	}
}

void Instance::writeToDB(void) {

}

static void recvCallback(std::string node, std::map<std::string, double> values) {
	cout << "Recevied: " << node << " with " << values.size() << " values" << endl;
	instance.receive(node, values);
}

static void sig_handler(int signo) {
	switch(signo) {
		case SIGINT:
		case SIGTERM:
			cerr << "Terminating ... " << endl;
			// instance.close();
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
