/* =============================================================================
 * 
 * Title:         Access to the TSL2561 Luminosity chip
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Based on the Software of Alexander Rüedlinger (luxruee) 
 *                from GitHub (https://github.com/lexruee/tsl2561)
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <string>

#include <cstdlib>
#include "tsl2561.c"

#include "tsl2561.hpp"


namespace sensors {


TSL2561::TSL2561(const char* i2c_device, int address) : Sensor(i2c_device, address) {
	this->tsl = tsl2561_init(address, i2c_device);
	if(this->tsl == NULL) this->_error = true;
	this->_visible = 0;
	this->_ir = 0;
}


TSL2561::TSL2561(const std::string i2c_device, int address)  : Sensor(i2c_device, address) {
	this->tsl = tsl2561_init(address, i2c_device.c_str());
	if(this->tsl == NULL) this->_error = true;
	this->_visible = 0;
	this->_ir = 0;
}


TSL2561::~TSL2561() {
	tsl2561_close(this->tsl);
}

	
int TSL2561::read() {
	if(this->tsl == NULL) return -1;
	tsl2561_luminosity(this->tsl, &this->_visible, &this->_ir);
	// TODO: Return status on error
	
	return 0;
}

}


