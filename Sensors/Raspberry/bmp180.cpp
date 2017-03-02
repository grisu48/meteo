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
 
 
#include <iostream>
#include <string>

#include <cstdlib>
#include "bmp180.c"

#include "bmp180.hpp"


namespace sensors {

BMP180::BMP180(const char* i2c_device, int address) : Sensor(i2c_device, address) {
	this->bmp = bmp180_init(address, i2c_device);
	if(this->bmp == NULL) this->_error = true;
	this->t = 0.0F;
	this->p = 0.0F;
	this->alt = 0.0F;
}


BMP180::BMP180(const std::string i2c_device, int address)  : Sensor(i2c_device, address) {
	this->bmp = bmp180_init(address, i2c_device.c_str());
	if(this->bmp == NULL) this->_error = true;
	this->t = 0.0F;
	this->p = 0.0F;
	this->alt = 0.0F;
}


BMP180::~BMP180() {
	bmp180_close(bmp);
}

	
int BMP180::read() {
	if(this->bmp == NULL) return -1;
	if(this->_error) return -1;

	// Unfortunately, currently there is no error report for the BMP180	
	this->t = bmp180_temperature(bmp);
	this->p = bmp180_pressure(bmp);
	this->alt = bmp180_altitude(bmp);
	
	return 0;
}

}

