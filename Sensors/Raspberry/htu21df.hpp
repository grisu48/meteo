/* =============================================================================
 * 
 * Title:         Access to the TSL2561 Luminosity chip
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Based on the Software of bbx10
 *                from GitHub (https://github.com/bbx10/htu21dflib)
 * 
 * =============================================================================
 */
  
 
#ifndef _METEO_HTU21DF_HPP
#define _METEO_HTU21DF_HPP
 
 
#include <iostream>
#include <string>

#include <cstdlib>

#include "sensor.hpp"


namespace sensors {

class HTU21DF : public Sensor {
private:
	std::string _device;
	int _address;
	
	// Last readings
	float t, h;
	
	
	int i2cfd;
	
	int init();
public:
	HTU21DF(const char* i2c_device, int address=DEVICE_ADDRESS);
	HTU21DF(const std::string i2c_device, int address=DEVICE_ADDRESS);
	virtual ~HTU21DF();
	
	int read(void);
	
	float temperature() { return this->t; }
	float humidity() { return this->h; }
	
	
	static const int DEVICE_ADDRESS = 0x40;
};


}



#endif
