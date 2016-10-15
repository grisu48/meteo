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


string Node::xml(void) const {
    stringstream ss;
    
    ss << "<Node name=\"" << this->_name << "\">";
    
    map<string, double> data(this->data);
    for(map<string, double>::iterator it = data.begin(); it != data.end(); ++it) {
        string name = it->first;
        const double value = it->second;
        
        ss << "<" << name << ">" << value << "</" << name << ">";
    }
    
    ss << "</Node>";
    
    
    return ss.str();
}

string RoomNode::toString(void) const {
	stringstream ss;

	ss << "Room[" << stationId() << "] -- " << lightPercent() << "% ligth; " << humidity() << " %. rel humidity at ";
	ss << temperature() << " degree Celcius";
	if (!isBatteryOk()) ss << "  -- Low battery!" << endl;

	return ss.str();
}


void RoomNode::setName(int id) {
	stringstream ss;
	ss << id;
	this->_name = ss.str();
}
