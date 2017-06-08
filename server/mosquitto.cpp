/* =============================================================================
 * 
 * Title:         Mosquitto interface for meteo
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2017 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Send data using mosquitto
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>


#include "mosquitto.hpp"


using namespace std;


void c_mosq_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str) {
	(void)mosq;
	Mosquitto *m_obj = (Mosquitto*)obj;
	m_obj->mosq_log_callback(level, str);
}

void c_mosq_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
	(void)mosq;
	Mosquitto *m_obj = (Mosquitto*)obj;
	m_obj->mosq_message_callback(message);
}


Mosquitto::Mosquitto() {
	
}

Mosquitto::Mosquitto(const std::string &hostname, const int port) : Mosquitto() {
	this->_hostname = hostname;
	this->_port = port;
}

Mosquitto::~Mosquitto() {
	this->close();
}




void Mosquitto::connect() {
	if(isConnected()) throw "Already connected";
	
	// Default settings
	bool clean_session = true;
	int keepalive = 60;
	
	mosquitto_lib_init();
	this->close();
	char *id = NULL;		// Random id
	mosq = mosquitto_new(id, clean_session, this);
	if(!mosq) {
		this->close();
		throw "Error setting up new mosquitto instance";
	}
	if(this->_username != "" || this->_password != "")
		mosquitto_username_pw_set(this->mosq, this->_username.c_str(), this->_password.c_str());
	
	mosquitto_log_callback_set(mosq, c_mosq_log_callback);
	mosquitto_message_callback_set(mosq, c_mosq_message_callback);
	
	if(mosquitto_connect(mosq, this->_hostname.c_str(), this->_port, keepalive)) {
		this->close();
		throw "Error connecting to host";
	}
	
	
	// Done
	connected = true;
}

void Mosquitto::close() {
	if(this->mosq != NULL) {
		mosquitto_destroy(this->mosq);
		this->mosq = NULL;
	}
}
void Mosquitto::subscribe(const std::string &topic) {
	if(!isConnected() || this->mosq == NULL) throw "Not connected";
	if(topic.size() == 0) return;
	
	mosquitto_subscribe(mosq, NULL, topic.c_str(), 0);
}


void Mosquitto::publish(const std::string &topic, const char* buffer, const size_t len) {
	if(!isConnected() || this->mosq == NULL) throw "Not connected";

	bool retain = false;		// Currently not used
	int ret = mosquitto_publish(mosq, NULL, topic.c_str(), len, buffer, 0, retain);
	if(ret != MOSQ_ERR_SUCCESS) throw "Error publishing";		// XXX: Better error handling
}

void Mosquitto::publish(const std::string &topic, const std::string &message) {
	this->publish(topic, message.c_str(), message.size());
	
}

void Mosquitto::mosq_log_callback(int level, const char *str) {
	(void)level;
	(void)str;
}
	
void Mosquitto::mosq_message_callback(const struct mosquitto_message *mosq_message) {
    string topic(mosq_message->topic);
    this->onMessageReceived(topic, (char*)mosq_message->payload, mosq_message->payloadlen);
}

void Mosquitto::start() {
    int rc = mosquitto_loop_start(this->mosq);
    if(rc != MOSQ_ERR_SUCCESS)
        throw "Error starting thread";
}

