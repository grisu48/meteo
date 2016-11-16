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
class DBNode;


/** METEO Node that collects data */
class Node {
protected:
	/** Name of the station */
	std::string _name;
	
	/** ID of the station */
	long _id;
	
	/** Timestamp of the last data push */
	long _timestamp;
	
	/** Data of the node */
	std::map<std::string, double> data;
	
public:
	Node();
	Node(std::string name);
	Node(long id, std::string name);
	/** Copy constructor */
	Node(const Node &node);
	/** Move constructor */
	Node(Node &&node);
	virtual ~Node();
	
	/** Get the name of the node */
	std::string name(void) const;
	/** Get the ID of the node */
	long id(void) const;
	/** Set the name of the node */
	void setName(std::string name);
	
	/** Push data for the given column*/
	double pushData(std::string column, const double value, const double alpha_avg = 0.9);
	
	/** Push the given values to the Node using pushData() */
	void pushData(std::map<std::string, double> &map, const double alpha_avg = 0.9);
	
	void addValues(std::map<std::string, double> &map);
	
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
	
	/** Assign operator */
	Node& operator=(const Node &node);
	
	/** Get XML of the node */
	std::string xml(void) const;
	
	std::string toString(void) const;
};


/** RoomNode packet*/
class RoomNode : public Node {
private:
	void setName(int id);
public:
	RoomNode(long id, float light, float humidity, float temperature, int battery) {
		this->_id = id;
		this->setName(id);
		this->data["light"] = light;
		this->data["humidity"] = humidity;
		this->data["temperature"] = temperature;
		this->data["battery"] = battery;
		
	}
	RoomNode(long id) {
		this->_id = id;
		this->setName(id);
		this->data["light"] = 0;
		this->data["humidity"] = 0;
		this->data["temperature"] = 0;
		this->data["battery"] = 0;
	}
	
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

/** Database node template */
class DBNode {
private:
	int _id;
	std::string _name;
	std::string _location;
	std::string _description;
	
public:
	DBNode();
	DBNode(const DBNode &node);
	DBNode(int id, std::string name, std::string location, std::string description);
	virtual ~DBNode();
	
	int id(void) const { return this->_id; }
	std::string name(void) const { return this->_name; }
	std::string location(void) const { return this->_location; }
	std::string description(void) const { return this->_description; }
	
	
};

#endif
