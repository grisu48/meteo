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
 
#ifndef _METEO_MCP9808_HPP
#define _METEO_MCP9808_HPP
 
 
#include <iostream>
#include <string>

#include <cstdlib>

#include "sensor.hpp"


namespace sensors {

class MCP9808 : public Sensor {
private:
	// Last readings
	float t;
	
	
	void* mcp9808;
public:
	MCP9808(const char* i2c_device, int address=DEVICE_ADDRESS);
	MCP9808(const std::string i2c_device, int address=DEVICE_ADDRESS);
	virtual ~MCP9808();
	
	int read(void);
	
	float temperature() { return this->t; }
	
	
	static const int DEVICE_ADDRESS = 0x18;
};

}



#endif
