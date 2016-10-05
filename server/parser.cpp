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
	String msg(input);
	if(msg.isEmpty()) return false;
	
	vector<String> split = msg.split(" ");
	String type = split[0];
	
	// Switch type of receiver
	if(type == "ROOM") {
		// ROOMNODE
		if(split.size() < 5) return false;
		
		// Clear items
		this->_node = split[1];		// ID of the node
		this->_values.clear();
		
		// Read ROOM items
		float light;                    // Light (0..255)
		float humidity;                 // Humidity in rel. %
		float temperature;              // Temperature in degree celcius
		int battery;                    // Battery status (0 = good, 1 = low voltage)
		
		light = atoi(split[2].c_str());
		humidity = atof(split[3].c_str());
		temperature = atof(split[4].c_str());
		battery = atoi(split[5].c_str());
		
		this->_values["light"] = light;
		this->_values["humidity"] = humidity;
		this->_values["temperature"] = temperature;
		this->_values["battery"] = battery;
		
		return true;
	} else {
		// Illegal package - Ignore it
		return false;
	}
}

void Parser::clear(void) {
	this->_node = "";
	this->_values.clear();
}
