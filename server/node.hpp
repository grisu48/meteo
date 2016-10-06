/* =============================================================================
 * 
 * Title:         Meteo Node
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Node collecting data
 * 
 * =============================================================================
 */
 
#ifndef _METEO_NODE_HPP_
#define _METEO_NODE_HPP_

#include <string>
#include <vector>
#include <map>

class Node;
class RoomNode;


/** METEO Node that collects data */
class Node {
private:
	/** Name of the station */
	std::string _name;
	
	/** Timestamp of the last data push */
	long _timestamp;
	
	/** Data of the node */
	std::map<std::string, double> data;
	
	/** Alive flag */
	bool _alive;
	
public:
	Node();
	Node(std::string name);
	virtual ~Node();
	
	/** Get the name of the node */
	std::string name(void) const;
	/** Set the name of the node */
	void setName(std::string name);
	
	/** Push data for the given column*/
	double pushData(std::string column, const double value, const double alpha_avg = 0.9);
	
	/** Clears all data */
	void clear(void);

	/** List of all data of the node */
	std::map<std::string, double> values(void);
	
	long timestamp(void) const { return this->_timestamp; }
	void setTimestamp(long timestamp) { this->_timestamp = timestamp; }
	
	/** Set alive status of this node */
	void setAlive(bool alive = true) { this->_alive = alive; }
	bool isAlive(void) const { return this->_alive; }
};


/** RoomNode packet*/
class RoomNode {
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
	RoomNode(int id, float light, float humidity, float temperature, int battery) {
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

	std::string toString(void) const;
};

#endif
