/*
 * @author 	Alexander Rüedlinger <a.rueedlinger@gmail.com>
 * @date 	07.03.2015
 *
 * A C driver for the sensor MPL115A2.
 *  
 */


void *mpl115a2_init(int address, const char* i2c_device_filepath);

void mpl115a2_close(void *_mpl);

float mpl115a2_pressure(void *_mpl);

float mpl115a2_temperature(void *_mpl);


void mpl115a2_read_data(void *_s, float *temperature, float *pressure);

