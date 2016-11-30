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
 
#ifndef _METEO_TSL2561_HPP
#define _METEO_TSL2561_HPP
 
 
#include <iostream>
#include <string>

#include <cstdlib>

#include "sensor.hpp"


namespace sensors {

class TSL2561 : public Sensor {
private:
	std::string _device;
	int _address;
	
	// Last readings
	int _visible, _ir;
	
	
	void* tsl;
public:
	TSL2561(const char* i2c_device, int address=DEVICE_ADDRESS);
	TSL2561(const std::string i2c_device, int address=DEVICE_ADDRESS);
	virtual ~TSL2561();
	
	int read(void);
	
	float visible() { return this->_visible; }
	float ir() { return this->_ir; }
	
	
	static const int DEVICE_ADDRESS = 0x39;
};


}



#endif
