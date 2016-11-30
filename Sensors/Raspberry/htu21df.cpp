/* =============================================================================
 * 
 * Title:         Access to the HTU21D-F Temperature/Humidity chip
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   Based on the Software of bbx10
 *                from GitHub (https://github.com/bbx10/htu21dflib)
 * 
 * =============================================================================
 */
  

#include <iostream>
#include <string>

#include <cstdlib>
#include <unistd.h>
#include "htu21dflib.c"

#include "htu21df.hpp"


namespace sensors {


HTU21DF::HTU21DF(const char* i2c_device, int address) : Sensor(i2c_device, address) {
	this->i2cfd = 0;
	int ret = init();
	if(ret < 0) {
		this->_error = true;
	}
	this->t = 0.0;
	this->h = 0.0;
}


HTU21DF::HTU21DF(const std::string i2c_device, int address)  : Sensor(i2c_device, address) {
	this->i2cfd = 0;
	int ret = init();
	if(ret < 0) {
		this->_error = true;
	}
	this->t = 0.0;
	this->h = 0.0;
}


HTU21DF::~HTU21DF() {
	if(this->i2cfd > 0)
		i2c_close(this->i2cfd);
}

int HTU21DF::init() {
	int fd;
	int rc;
	
	fd = i2c_open(this->_device.c_str());
	if(fd < 0)
		return -1;		// i2c_open failed
	rc = htu21df_init(fd, this->_address);
	if(rc < 0) {
		i2c_close(fd);
		return -2;		// i2c_init failed
	}
	
	this->i2cfd = fd;
	return 0;
}
	
int HTU21DF::read() {
	int rc;
	int ret = 0;
	
	rc = htu21df_read_temperature(this->i2cfd, &this->t);
	if(rc != 0) ret = -1;
	rc = htu21df_read_humidity(this->i2cfd, &this->h);
	if(rc != 0) ret += -2;
	
	return ret;
}



}


