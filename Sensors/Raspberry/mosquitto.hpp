/**
  * Mosquitto C++ Wrapper
  * (C) 2017, Felix Niederwanger
  * MIT License
  */

#ifndef _MOSQUITTO_HPP_
#define _MOSQUITTO_HPP_

#include <string>
#include <sstream>

#include <unistd.h>
#include <mosquitto.h>


class Mosquitto {
private:
    struct mosquitto *mosq;

	volatile bool running;
public:
	Mosquitto();
	virtual ~Mosquitto();
	
	/** Enable tls
	  * @param cacert CaCert file
	  * @param certfile Client certificate file. usually a .crt file
	  * @param keyfile Client key file, usually a .key file
	  */
	void set_tls(const char* cacert, const char* certfile, const char* keyfile);
	/** Enable or disable insecure TLS connections (No certificate check) */
	void set_tls_insecure(bool enabled);
	
	/** Set username and password for login */
	void set_username_password(const char* username, const char* password);
	
	/** Disconnects the client */
	void close();
	
	/** Subscribe to the given topic */
	void subscribe(const std::string &topic);
	
	void publish(const std::string &topic, const std::string &message);
	
	/**
	  * Connects to the given remote host
	  * @param remote Remote host where to connect to
	  * @param port Remote port. 1883 for unencrypted, 8883 usually for encrypted traffic
	  * @param aliveDelay Delay in seconds for pings for the connection to stay alive
	  */
	void connect(const char* remote, const int port = 1883, const int aliveDelay = 30);

	/** Method called when the client is connected */
	virtual void onConnected() {}
	/** Method called then an error occurred */
	virtual void onError(const char* msg) { (void)msg; }
	/** Method called when a new message arrives */
	virtual void onMessage(std::string topic, std::string message) { (void)topic; (void)message; }
	
	/** Loop through messages. This call usually blocks until the connection is closed
	  *@param tryReconnect if true, the client tries to reconnect if an error occurs */
	void loop(const bool tryReconnect = true);
	
	/** Starts the loop as background thread */
	void loopStart();
	
	/** Cleanup mosquitto library. This call should be called before program termination */
	static void cleanup_library();
};


void mosquitto_callback_on_connect(struct mosquitto *mosq_obj, void *obj, int rc) {
	(void)mosq_obj;
	Mosquitto *mosq = (Mosquitto*)obj;
	
	switch(rc) {
	case 0:
		// Connected
		mosq->onConnected();
		break;
	case 1:
		mosq->onError("Connection refused (unacceptable protocol version)");
		break;
	case 2:
		mosq->onError("Connection refused (identifier rejected)");
		break;
	case 3:
		mosq->onError("Connection refused (broker unavailable)");
		break;
	case 5:
		mosq->onError("Connection refused (Username/Password wrong)");
		break;
	default:
		std::stringstream ss ;
		ss << "Connection failed (Error code " << rc << ")";
		std::string msg = ss.str();
		mosq->onError(msg.c_str());
		break;
	};
}

static void mosquitto_callback_on_message(struct mosquitto *mosq_obj, void *obj, const struct mosquitto_message *mosq_message) {
	(void)mosq_obj;
	Mosquitto *mosq = (Mosquitto*)obj;
	std::string topic(mosq_message->topic);
	
	// Build message
	std::stringstream ss;
	const int len = mosq_message->payloadlen;
	char* payload = (char*)mosq_message->payload;
	for(int i=0;i<len;i++) ss << (char)(payload[i]);
	
	mosq->onMessage(topic, ss.str());
}




Mosquitto::Mosquitto() {
    mosquitto_lib_init();
    this->running = false;
    this->mosq = mosquitto_new(NULL, true, this);
    if(this->mosq == NULL) throw "Error creating mosquitto instance";
    
    
	mosquitto_connect_callback_set(this->mosq, &mosquitto_callback_on_connect);
	mosquitto_message_callback_set(this->mosq, &mosquitto_callback_on_message);
}


Mosquitto::~Mosquitto() {
    mosquitto_destroy(this->mosq);
}

static int mosquitto_hpp_pw_callback(char *buf, int size, int rwflag, void *userdata) {
	(void)buf;
	(void)size;
	(void)rwflag;
	(void)userdata;
	return -1;
}

void Mosquitto::set_tls(const char* cacert, const char* certfile, const char* keyfile) {
	if(mosquitto_tls_set(this->mosq, cacert, NULL, certfile, keyfile, &mosquitto_hpp_pw_callback) != MOSQ_ERR_SUCCESS) 
		throw "Error setting tls settings";
}

void Mosquitto::set_tls_insecure(bool enabled) {
	int value = (enabled?1:0);
	if(mosquitto_tls_insecure_set(this->mosq, value) != MOSQ_ERR_SUCCESS) 
		throw "TLS insecure set failed";
}

void Mosquitto::set_username_password(const char* username, const char* password) {
	if(mosquitto_username_pw_set(this->mosq, username, password) != MOSQ_ERR_SUCCESS)
		throw "Error setting username and password";
}


void Mosquitto::connect(const char* remote, const int port, const int aliveDelay) {	
	int ret = mosquitto_connect(this->mosq, remote, port, aliveDelay);
	if(ret != MOSQ_ERR_SUCCESS)
		throw mosquitto_strerror(ret);
	this->running = true;
}
	
void Mosquitto::subscribe(const std::string &topic) {
	const int qos = 0;
	int ret = mosquitto_subscribe(this->mosq, NULL, topic.c_str(), qos);
	if(ret != MOSQ_ERR_SUCCESS)
		throw mosquitto_strerror(ret);
}

void Mosquitto::publish(const std::string &topic, const std::string &message) {
	const char *payload = message.c_str();
	const int len = (int)message.size();
	
	const int qos = 0;
	int ret = mosquitto_publish(this->mosq, NULL, topic.c_str(), len, (const void*)payload, qos, false);
	if(ret != MOSQ_ERR_SUCCESS)
		throw mosquitto_strerror(ret);
}

	
void Mosquitto::close() {
	this->running = false;
	mosquitto_disconnect(this->mosq);
}

void Mosquitto::loop(const bool tryReconnect) {
	int errorCounter = 0;
	
	// XXX: Consider switching to mosquitto_loop_forever
	
	do {
		const int ret = mosquitto_loop(this->mosq, 10000, 10);
		if(!this->running) return;
		
		if(ret != MOSQ_ERR_SUCCESS) {
			
			// Sleep for some time, if necessary
			if(errorCounter > 1) {
				int duration = errorCounter*errorCounter;
				if(duration > 60 || duration <= 0) duration = 60;
				::sleep(duration);		// Wait some time before trying again
			}
			
			if(mosquitto_reconnect(mosq) == MOSQ_ERR_SUCCESS) {
				// Reconnect successfull
				continue;
			} else {
				// Reconnect failed. Increase counter
				errorCounter++;
			}
		} else
			errorCounter = 0;
	} while(this->running && tryReconnect);
}

void Mosquitto::cleanup_library() {
    mosquitto_lib_cleanup();
}


void Mosquitto::loopStart() {
	int rc = mosquitto_loop_start(this->mosq);
	if(rc != MOSQ_ERR_SUCCESS)
		throw mosquitto_strerror(rc);
}

#endif
