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
 
#ifndef _METEO_SENSOR_HPP
#define _METEO_SENSOR_HPP
 
 
#include <iostream>
#include <string>

#include <cstdlib>
#include <unistd.h>


namespace sensors {


/**
  * Abstract superclass for all Sensors
  */
  
class Sensor {
private:
	/** Device name (default: "/dev/i2c-1")*/
	std::string _device;
	/** I2C Address */
	int _address;
	
protected:
	/** Error flag */
	bool _error;
public:
	/** Initialize sensor */
	Sensor(const char* i2c_device, int address);
	/** Initialize sensor */
	Sensor(const std::string i2c_device, int address);
	virtual ~Sensor();
	
	/** @returns the device name */
	std::string device(void);
	/** @returns the I2C-address for this sensor */
	int address(void);
	
	/** Reads the sensor
	  * @returns 0 on success, a non-zero value on error
	  */
	virtual int read(void) = 0;

#if 0
	/**
	  * (Re)Initialize the device
	  * @returns 0 on success, a non-zero value on error
	*/
	virtual int init(void) = 0;
#endif
	
	/**
	  * Get the error flag and sets it to false
	  * @returns true if an error was detected
	*/
	virtual bool isError(void);

	/** Default I2C defive to take */
	static constexpr const char* DEFAULT_I2C_DEVICE = (const char*)"/dev/i2c-1";
};


}



#endif
