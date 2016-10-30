/* =============================================================================
 * 
 * Title:         Parser class
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
#include <string>
#include <vector>
#include <map>
 
#include "parser.hpp"
 
using namespace std;
using flex::String;


Parser::Parser() {}
Parser::~Parser() {}

bool Parser::parse(std::string input) {
	this->clear();
	
	
	String msg(input);
	if(msg.isEmpty()) return false;
	
	
		// Plain text node
		vector<String> split = msg.split(" ");
		String type = split[0];
	
	// Switch type of receiver
	if(type == "ROOM") {
		// ROOMNODE
		/* The expected input value is ROOM <ID> <LIGHT> <HUMIDITY> <TEMPERATRE> <BATTERY> */
		/* e.g.  "ROOM 4 100 50 20 0" for Roomnode 4 (id = 4), with light = 100, humidity = 50 % rel, temperature = 20 degree C, and good battery (battery = 0) */
		if(split.size() < 5) return false;
	
		// Clear items
		this->_id = atol(split[1].c_str());		// ID of the node
		this->_name = "ROOM " + ::to_string(this->_id);
		this->_values.clear();
	
		// Read ROOM items
		float light;                    // Light (0..255)
		float humidity;                 // Humidity in rel. %
		float temperature;              // Temperature in degree celcius
		int battery;                    // Battery status (0 = good, 1 = low voltage)
	
		light = atoi(split[2].c_str());
		humidity = atof(split[3].c_str());
		temperature = atof(split[4].c_str()) / 10.0F;
		battery = atoi(split[5].c_str());
	
		this->_values["light"] = light;
		this->_values["humidity"] = humidity;
		this->_values["temperature"] = temperature;
		this->_values["battery"] = battery;
	
		return true;
	} else if(type == "NODE") {
		// Special definition node
		/* Definition: NODE <ID> [NAME=VALUE] separated by ',' */
		/* e.g. NODE 6 temperature = 20, humidity=50 */
		
		split = msg.split(" ", 3);
		if (split.size() < 2) return false;		// Not enough arguments
		
		
		// Clear items
		this->_id = atol(split[1].c_str());		// ID of the node
		this->_name = "NODE " + ::to_string(this->_id);
		this->_values.clear();
		
		if (split.size() > 2) {
			// Extract values
			split = split[2].split(",");
			
			// Iterate over elements
			for(vector<String>::iterator it = split.begin(); it != split.end(); ++it) {
				vector<String> sp = (*it).split("=");
				if(sp.size() < 2) return false;		// Illegal argument
				
				String name = sp[0].trim();
				String value = sp[1].trim();
				
				if(name.isEmpty()) return false;
				if(value.isEmpty()) return false;
				
				this->_values[name] = value.toDouble();
			}
			
		}
		
		return true;
		
	} else {
		// Illegal package - Ignore it
		return false;
	}
}

void Parser::clear(void) {
	this->_name = "";
	this->_id = 0L;
	this->_values.clear();
}

Node Parser::node(void) {
	Node result(this->_id, this->_name);
	result.addValues(this->_values);
	return result;
}
