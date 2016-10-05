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

#endif
