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
protected:
	/** Name of the station */
	std::string _name;
	
	/** Timestamp of the last data push */
	long _timestamp;
	
	/** Data of the node */
	std::map<std::string, double> data;
	
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
	std::map<std::string, double> values(void) const;
	
	long timestamp(void) const { return this->_timestamp; }
	void setTimestamp(long timestamp) { this->_timestamp = timestamp; }
	
	/** Get column contents or 0, if not found */
	double get(const char* column) const;
	
	/** Get column contents or 0, if not found */
	double operator[](const char* column) const;
	
	/** Get XML of the node */
	std::string xml(void) const;
};


/** RoomNode packet*/
class RoomNode : public Node {
private:
	/** ID of the station */
	int _id;
	
	void setName(int id);
public:
	RoomNode(int id, float light, float humidity, float temperature, int battery) {
		this->_id = id;
		this->setName(id);
		this->data["light"] = light;
		this->data["humidity"] = humidity;
		this->data["temperature"] = temperature;
		this->data["battery"] = battery;
		
	}
	RoomNode(int id) {
		this->_id = id;
		this->setName(id);
		this->data["light"] = 0;
		this->data["humidity"] = 0;
		this->data["temperature"] = 0;
		this->data["battery"] = 0;
	}

	/** The ID of the station */
	int stationId(void) const { return this->_id; }
	
	/** Light (value from 0 .. 255) */
	float light(void) const { return (float)this->get("light"); }
	/** Light value in percent (0..100) */
	float lightPercent(void) const { return this->light()/2.55; }
	/** Humditiy in rel. % */
	float humidity(void) const { return (float)this->get("humidity"); }
	/** Temperature in degree celcius*/
	float temperature(void) const { return (float)this->get("temperature"); }
	/** Battery status - 0 = good, 1 = low voltage */
	int battery(void) const { return (int)this->get("battery"); }
	/** Battery status - 0 = good, 1 = low voltage */
	bool isBatteryOk(void) const { return this->battery() == 0; }

	std::string toString(void) const;
};

#endif
