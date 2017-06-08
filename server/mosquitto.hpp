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

#ifndef _METEO_MOSQUITTO_HPP_
#define _METEO_MOSQUITTO_HPP_

#include <string>
#include <vector>
#include <map>

#include <mosquitto.h>
#include <unistd.h>
#include <pthread.h>



class Mosquitto {
private:
	struct mosquitto *mosq = NULL;
	
	std::string _username = "";
	std::string _password = "";
	std::string _hostname = "127.0.0.1";
	int _port = 1883;
	
	/** Connected flag */
	bool connected = false;
	
	void(*recv_callback)(const std::string &topic, char* buffer, size_t len) = NULL;
	
protected:
	
public:
	Mosquitto();
	Mosquitto(const std::string &hostname, const int port = 1883);
	virtual ~Mosquitto();
	
	void setUsername(const std::string &username) { this->_username = username; }
	void setPassword(const std::string &password) { this->_password = password; }
	void setHostname(const std::string &hostname) { this->_hostname = hostname; }
	void setPort(const int port) { this->_port = port; }
	
	bool isConnected(void) const { return this->connected; }
	
	/** Connects the mosquitto client */
	void connect(void);
	
	/** Close connection */
	void close();
	
	/** Subscribe to the given topic */
	void subscribe(const std::string &topic);
	
	void publish(const std::string &topic, const char* buffer, const size_t len);
	void publish(const std::string &topic, const std::string &message);
	
	
	/** Start the looper */
	void start();
	
	
	/** Logging callback */
	virtual void mosq_log_callback(int level, const char *str);
	
	/** Message callback */
	virtual void mosq_message_callback(const struct mosquitto_message *message);
	
	void setReceiveCallback(void(*callback)(const std::string &topic, char* buffer, size_t len)) {
		this->recv_callback = callback;
	}
	
	/** Called when a new message has been received */
	virtual void onMessageReceived(const std::string &topic, char* buffer, size_t len) {
		if(recv_callback != NULL)
			recv_callback(topic, buffer, len);
	}
};





#endif
