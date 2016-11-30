/*
 * @author 	Alexander Rüedlinger <a.rueedlinger@gmail.com>
 * @date 	12.03.2015
 *
 * A C driver for the sensor lm75.
 *  
 */


void *lm75_init(int address, const char* i2c_device_filepath);

void lm75_close(void *_s);

float lm75_temperature(void *_s);

