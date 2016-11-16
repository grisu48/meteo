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
 
#include <sstream>
 
#include "node.hpp"

using namespace std;

Node::Node() {
	this->_timestamp = 0L;
	this->_id = 0L;
}
Node::Node(std::string name) {
	this->_name = name;
	this->_timestamp = 0L;
	this->_id = 0L;
}

Node::Node(long id, std::string name) {
	this->_name = name;
	this->_timestamp = 0L;
	this->_id = id;
}


Node::Node(const Node &node) {
	this->_id = node._id;
	this->_name = node._name;
	this->_timestamp = node._timestamp;
	this->data = map<string, double>(node.data);
}

Node::Node(Node &&node) {
	this->_id = node._id;
	this->_name = node._name;
	this->_timestamp = node._timestamp;
	this->data = node.data;
	node._name = "";
	node._timestamp = 0L;
	node._id = 0;
	node.data.clear();
}

Node::~Node() {}

std::string Node::name(void) const { return this->_name; }
long Node::id(void) const { return this->_id; }

void Node::setName(std::string name) {
	this->_name = name;
}


double Node::pushData(std::string column, const double value, const double alpha_avg) {
	// Check if existing
	if(this->data.find(column) != this->data.end()) {
		// Push averaged value
		double val = this->data[column];
		val = alpha_avg * val + (1.0 - alpha_avg) * value;
		this->data[column] = val;
		return val;
	} else {
		// Just push data
		this->data[column] = value;
		return value;
	}
}

void Node::pushData(std::map<std::string, double> &values, const double alpha_avg) {
	for(map<string, double>::iterator it = values.begin(); it != values.end(); ++it)
		this->pushData(it->first, it->second, alpha_avg);
}

void Node::addValues(map<string, double> &values) {
	for(map<string, double>::iterator it = values.begin(); it != values.end(); ++it) {
		this->data[it->first] = it->second;
	}
}

void Node::clear(void) {
	this->data.clear();
}

map<string, double> Node::values(void) const {
	map<string, double> ret(this->data);
//	for(map<string, double>::iterator it = this->data.begin(); it != this->data.end(); it++) 
//		ret[it->first] = it->second;
	return ret;
}

double Node::get(const char* column) const {
	if(column == NULL) return 0;
	// Search for item
	string sColumn(column);
	if(this->data.find(sColumn) == this->data.end()) 
		return 0;
	else
		return this->data.at(sColumn);
}


double Node::operator[](const char* column) const { return this->get(column); }

Node& Node::operator=(const Node &node) {
	this->_id = node._id;
	this->_name = node._name;
	this->_timestamp = node._timestamp;
	this->data = map<string, double>(node.data);
	return (*this);
}

string Node::xml(void) const {
    stringstream ss;
    
    ss << "<Node";
    ss << " id=\"" << this->id() << '\"';
    if(this->_name.length() > 0) ss << " name=\"" << this->_name << "\"";
    
    
    map<string, double> data(this->data);
    for(map<string, double>::iterator it = data.begin(); it != data.end(); ++it) {
        string name = it->first;
        const double value = it->second;
        
        ss << " " << name << "=\"" << value << "\"";
    }
    
    ss << " />";
    
    
    return ss.str();
}


string Node::toString(void) const {
	stringstream ss;
	
	ss << "Node[" << id();
	if(this->_name.size() > 0) ss << ", \"" << this->_name << '\"';
	ss << "]";
	map<string,double> data(this->data);
	for(map<string,double>::iterator it = data.begin(); it!=data.end(); ++it)
		ss << " " << it->first << " = " << it->second;
	
	return ss.str();
}

string RoomNode::toString(void) const {
	stringstream ss;

	ss << "Room[" << id() << "] -- " << lightPercent() << "% ligth; " << humidity() << " %. rel humidity at ";
	ss << temperature() << " degree Celcius";
	if (!isBatteryOk()) ss << "  -- Low battery!" << endl;

	return ss.str();
}


void RoomNode::setName(int id) {
	stringstream ss;
	ss << id;
	this->_name = ss.str();
}




DBNode::DBNode() {}

DBNode::DBNode(int id, std::string name, std::string location, std::string description) {
	this->_id = id;
	this->_name = name;
	this->_location = location;
	this->_description = description;
}

DBNode::DBNode(const DBNode &node) {
	this->_id = node._id;
	this->_name = node._name;
	this->_location = node._location;
	this->_description = node._description;
}

DBNode::~DBNode() {}

