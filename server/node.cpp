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
 
#include "node.hpp"

using namespace std;

Node::Node() {
	this->_timestamp = 0L;
}
Node::Node(std::string name) {
	this->_name = name;
	this->_timestamp = 0L;
}

Node::~Node() {}

std::string Node::name(void) const { return this->_name; }
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

void Node::clear(void) {
	this->data.clear();
}

map<string, double> Node::values(void) {
	map<string, double> ret;
	for(map<string, double>::iterator it = this->data.begin(); it != this->data.end(); it++) 
		ret[it->first] = it->second;
	return ret;
}

