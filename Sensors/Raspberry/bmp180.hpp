/* =============================================================================
 * 
 * Title:         Access to the BMP180 and BMP085 chips
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Based on the Software of Alexander Rüedlinger (luxruee) 
 *                from GitHub (https://github.com/lexruee/bmp180)
 * 
 * =============================================================================
 */
 
#ifndef _METEO_BMP180_HPP
#define _METEO_BMP180_HPP
 
 
#include <iostream>
#include <string>

#include <cstdlib>

#include "sensor.hpp"


namespace sensors {

class BMP180 : public Sensor {
private:
	// Last readings
	float t,p,alt;
	
	
	void* bmp;
public:
	BMP180(const char* i2c_device, int address=DEVICE_ADDRESS);
	BMP180(const std::string i2c_device, int address=DEVICE_ADDRESS);
	virtual ~BMP180();
	
	int read(void);
	
	float temperature() { return this->t; }
	float pressure() { return this->p; }
	float altitude() { return this->alt; }
	
	
	static const int DEVICE_ADDRESS = 0x77;
};


}



#endif
