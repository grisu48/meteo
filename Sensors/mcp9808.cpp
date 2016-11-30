/* =============================================================================
 * 
 * Title:         Access to the TSL2561 Luminosity chip
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Based on the Software of Alexander Rüedlinger (luxruee) 
 *                from GitHub (https://github.com/lexruee/mcp9808)
 * 
 * =============================================================================
 */

 
 
#include <iostream>
#include <string>

#include <cstdlib>
#include "mcp9808.c"

#include "mcp9808.hpp"


namespace sensors {


MCP9808::MCP9808(const char* i2c_device, int address) : Sensor(i2c_device, address) {
	this->mcp9808 = mcp9808_init(address, i2c_device);
	if(this->mcp9808 == NULL) this->_error = true;
	this->t = 0.0F;
}


MCP9808::MCP9808(const std::string i2c_device, int address)  : Sensor(i2c_device, address) {
	this->mcp9808 = mcp9808_init(address, i2c_device.c_str());
	if(this->mcp9808 == NULL) this->_error = true;
	this->t = 0.0F;
}


MCP9808::~MCP9808() {
	mcp9808_close(this->mcp9808);
}

	
int MCP9808::read() {
	if(this->mcp9808 == NULL) return -1;
	
	this->t = mcp9808_temperature(this->mcp9808);
	// TODO: Return status on error
	return 0;
}

}


