/* =============================================================================
 * 
 * Title:         General Sensor classes
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 

#include <string>

#include "sensor.hpp"

namespace sensors {

Sensor::Sensor(const char* i2c_device, int address) {
	this->_device = std::string(i2c_device);
	this->_address = address;
	this->_error = false;
}


Sensor::Sensor(const std::string i2c_device, int address) {
	this->_device = std::string(i2c_device);
	this->_address = address;
	this->_error = false;
}


Sensor::~Sensor() {
	
}

bool Sensor::isError(void) {
	const bool result = this->_error;
	this->_error = false;
	return result;
}

std::string Sensor::device() { return this->_device; }
int Sensor::address() { return this->_address; }


}

