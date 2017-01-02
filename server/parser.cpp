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
#include <sstream>
#include <vector>
#include <map>
 
#include "parser.hpp"
 
using namespace std;
using flex::String;


Parser::Parser() {}
Parser::~Parser() {}

bool Parser::parse(std::string input) {
	this->clear();
	if(input.size() == 0) return false;
	
	String msg = String(input).trim();
	if(msg.isEmpty()) return false;

	// Check for XML
	if(msg.startsWithIgnoreCase("<meteo") && msg.endsWith("/>")) {
		// XXX: This section was written in a haste, and could need some attention!
		// XML input of a METEO station
		
		String rem = msg.substr(6).trim();
		rem = rem.left(rem.size()-2);
		
		const size_t len = rem.size();
		
		this->_id = 0;
		this->_values.clear();
		
		// Split tokens
		vector<String> tokens;
		{
			bool inString = false;
			stringstream ss;
			for(size_t i=0;i<len;i++) {
				char c = rem.at(i);
				
				if(inString) {
					if (c== '\"') {
						inString = false;
						continue;
					} else
						ss << c;
				} else {
					if (c== '\"') {
						inString = true;
						continue;
					}
					
					if(::isspace(c)) {
						String token = String(ss.str()).trim();
						ss.str("");
						if(!token.isEmpty()) tokens.push_back(token);
					} else
						ss << c;
				}
			}
			
			// Last token
			String token = String(ss.str()).trim();
			if(!token.isEmpty()) tokens.push_back(token);
		}
		
		// Parse token
		for(vector<String>::iterator it = tokens.begin(); it != tokens.end(); it++) {
			String token = *it;
			
			size_t index = token.find('=');
			if(index == string::npos || index == 0) continue;
			String name = token.left(index);
			String value = token.mid(index+1);
			if(name.isEmpty() || value.isEmpty()) continue;
			
			const double f_val = atof(value.c_str());
			
			// Just keep the relevant data
			name = name.toLowercase();
			if(name == "temperature" || name == "humidity" || name == "pressure")
				this->_values[name] = f_val;
			else if(name == "station") {
				this->_id = atol(value.c_str());
			}
		}
		
		return true;
	}
	
	
	
	
	
	/* ==== Plain text node ================================================= */
	vector<String> split = msg.split(" ");
	String type = split[0].toUppercase();
	
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
