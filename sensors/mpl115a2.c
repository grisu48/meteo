/*
 * @author 	Alexander Rüedlinger <a.rueedlinger@gmail.com>
 * @date 	07.03.2015
 *
 * A C driver for the sensor MPL115A2.
 *
 */

#ifndef __MPL115A2__
#define __MPL115A2__
#include <stdint.h>
#include "mpl115a2.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <math.h>
#endif

typedef struct {
	/* file descriptor */
	int file;

	/* i2c device address */
	int address;

	/* i2c device file path */
	char *i2c_device;

	/* coefficients */
	float a0;
	float b1;
	float b2;
	float c12;

} mpl115a2_t;

/*
 * Registers
 */
#define MPL115A2_REG_PRESSURE_MSB	 	0x00
#define MPL115A2_REG_TEMPERATURE_MSB	0x02

/*
 * Commands
 */
#define MPL115A2_CMD_CONVERSION			0x12


/*
 * Coefficients
 */
#define MPL115A2_REG_A0_MSB		 		0x04
#define MPL115A2_REG_B1_MSB		 		0x06
#define MPL115A2_REG_B2_MSB		 		0x08
#define MPL115A2_REG_C12_MSB			0x0A


#define TO_S(x)	(mpl115a2_t*) x

//#define __MPL115A2__DEBUG__
#ifdef __MPL115A2__DEBUG__
#define DEBUG(...)	printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif


/*
 * Prototypes for helper functions
 */

void mpl115a2_read_coeff(void *_s);
int mpl115a2_set_addr(void *_s);
void mpl115a2_init_error_cleanup(void *_s);


/*
 * Implemetation of the helper functions
 */


/*
 * Reads the calibration coefficients from the MPL115A2 sensor.
 *
 * @param mpl115a2 sensor
 */
void mpl115a2_read_coeff(void *_s) {
	mpl115a2_t *s = TO_S(_s);

	uint8_t a0_msb = (uint8_t) i2c_smbus_read_byte_data(s->file, MPL115A2_REG_A0_MSB);
	uint8_t a0_lsb = (uint8_t) i2c_smbus_read_byte_data(s->file, MPL115A2_REG_A0_MSB + 1);

	uint8_t b1_msb = (uint8_t) i2c_smbus_read_byte_data(s->file, MPL115A2_REG_B1_MSB);
	uint8_t b1_lsb = (uint8_t) i2c_smbus_read_byte_data(s->file, MPL115A2_REG_B1_MSB + 1);

	uint8_t b2_msb = (uint8_t) i2c_smbus_read_word_data(s->file, MPL115A2_REG_B2_MSB);
	uint8_t b2_lsb = (uint8_t) i2c_smbus_read_word_data(s->file, MPL115A2_REG_B2_MSB + 1);

	uint8_t c12_msb = (uint8_t) i2c_smbus_read_byte_data(s->file, MPL115A2_REG_C12_MSB);
	uint8_t c12_lsb = (uint8_t) i2c_smbus_read_byte_data(s->file, MPL115A2_REG_C12_MSB + 1);

	// signs of the coeffs. are correct because we use int16_t ints.
	int16_t a0, b1, b2, c12;

	// convert to integers
	a0 = (a0_msb<<8) + a0_lsb;
	b1 = (b1_msb<<8) + b1_lsb;
	b2 = (b2_msb<<8) + b2_lsb;
	c12 = (c12_msb<<8) + c12_lsb;


	// convert integers to floats and store them
	s->a0 = a0 / 8.0f; 			// 2^-3
	s->b1 = b1/ 8192.0f; 		// 2^-13
	s->b2 = b2 / 16384.0f; 		// 2^-14
	s->c12 = c12 / 16777216.0f; // 2^-24

	DEBUG("c12: %#x, c12{1:0} %#x\n", c12_lsb, c12_lsb & 0x03);
	DEBUG("a0_msb: %#x, a0_lsb: %#x\n ", a0_msb, a0_lsb);
	DEBUG("b1_msb: %#x, b1_lsb: %#x\n ", b1_msb, b1_lsb);
	DEBUG("b2_msb: %#x, b2_lsb: %#x\n ", b2_msb, b2_lsb);
	DEBUG("c12_msb: %#x,c12_lsb: %#x\n ", c12_msb, c12_lsb);

	DEBUG("a1: %f\n ", s->a0);
	DEBUG("b1: %f\n ", s->b1);
	DEBUG("b2: %f\n ", s->b2);
	DEBUG("c12: %f\n ", s->c12);
}


/*
 * Sets the address for the i2c device file.
 *
 * @param mpl115a2 sensor
 */
int mpl115a2_set_addr(void *_s) {
	mpl115a2_t* s = TO_S(_s);
	int error;

	if((error = ioctl(s->file, I2C_SLAVE, s->address)) < 0)
		DEBUG("error: ioctl() failed\n");

	return error;
}



/*
 * Frees allocated memory in the init function.
 *
 * @param mpl115a2 sensor
 */
void mpl115a2_init_error_cleanup(void *_s) {
	mpl115a2_t* s = TO_S(_s);

	if(s->i2c_device != NULL) {
		free(s->i2c_device);
		s->i2c_device = NULL;
	}

	free(s);
	s = NULL;
}


/*
 * Implementation of the interface functions.
 */


/**
 * Creates a MPL115A2 sensor object.
 *
 * @param i2c device address
 * @param i2c device file path
 * @return mpl115a2 sensor
 */
void *mpl115a2_init(int address, const char* i2c_device_filepath) {
	DEBUG("device: init using address %#x and i2cbus %s\n", address, i2c_device_filepath);

	void *_s = malloc(sizeof(mpl115a2_t));
	if(_s == NULL)  {
		DEBUG("error: malloc returns NULL pointer\n");
		return NULL;
	}

	mpl115a2_t *s = TO_S(_s);
	s->address = address;

	s->i2c_device = (char*) malloc(strlen(i2c_device_filepath) * sizeof(char));
	if(s->i2c_device == NULL) {
		DEBUG("error: malloc returns NULL pointer!\n");
		mpl115a2_init_error_cleanup(s);
		return NULL;
	}

	// copy string
	strcpy(s->i2c_device, i2c_device_filepath);

	// open i2c device
	int file;
	if((file = open(s->i2c_device, O_RDWR)) < 0) {
		DEBUG("error: %s open() failed\n", s->i2c_device);
		mpl115a2_init_error_cleanup(s);
		return NULL;
	}
	s->file = file;
	if(mpl115a2_set_addr(s) < 0) {
		mpl115a2_init_error_cleanup(s);
		return NULL;
	}
	mpl115a2_read_coeff(s);

	DEBUG("device: open ok\n");
	return _s;
}



/**
 * Closes a MPL115A2 object.
 *
 * @param mpl115a2 sensor
 */
void mpl115a2_close(void *_s) {
	if(_s == NULL) {
		return;
	}
	
	DEBUG("close device\n");
	mpl115a2_t *s = TO_S(_s);

	if(close(s->file) < 0)
		DEBUG("error: %s close() failed\n", s->i2c_device);

	free(s->i2c_device); // free string
	s->i2c_device = NULL;
	free(s); // free structure
	_s = NULL;
}



/**
 * Read temperature and pressure value from the MPL115A2 sensor.
 *
 * @param mpl115a2 sensor
 * @param temperature
 * @param pressure
 */
void mpl115a2_read_data(void *_s, float *temperature, float *pressure) {
	mpl115a2_t *s = TO_S(_s);

	if(i2c_smbus_write_byte_data(s->file, MPL115A2_CMD_CONVERSION, 0x00) < 0) {
		DEBUG("error: i2c_smbus_write_byte\n");
	}

	usleep(5 * 1000); // 5 ms

	uint8_t pressure_msb, pressure_lsb, temperature_msb, temperature_lsb;

	uint16_t temperature_word = i2c_smbus_read_word_data(s->file, MPL115A2_REG_TEMPERATURE_MSB);
	uint16_t pressure_word = i2c_smbus_read_word_data(s->file, MPL115A2_REG_PRESSURE_MSB);

	// note: arm uses big endian, but i2c_smbus_x functions assume little endian.
	// word  = 0,1,2,3,4,5,6,7, 8,9,10,11,12,13,14,15
	//        [     lsb       ][         msb         ]
	temperature_msb = (temperature_word & 0x00FF); // extract msb byte
	temperature_lsb = (temperature_word & 0xFF00) >> 8;

	pressure_msb = (pressure_word & 0x00FF); // extract lsb byte
	pressure_lsb = (pressure_word & 0xFF00) >> 8;

	uint16_t raw_temperature = ((temperature_msb << 8) + temperature_lsb) >> 6;
	uint16_t raw_pressure = ((pressure_msb << 8) + pressure_lsb) >> 6;

	float pressure_comp = s->a0 + (s->b1 + s->c12 * raw_temperature) * raw_pressure + s->b2 * raw_temperature;

	// black magic temperature formula: http://forums.adafruit.com/viewtopic.php?f=25&t=34787
	// thx @park
	*temperature = ((float) raw_temperature) * -0.1707f + 112.27f;
	*pressure =  ((pressure_comp / 15.737f) + 50.0f)*1000;

	DEBUG("tmp: %f ,tmp_msb: %#x, tmp_lsb: %#x\n ", *temperature, temperature_msb, temperature_lsb);
	DEBUG("pre: %f ,pre_msb: %#x, pre_lsb: %#x, raw: %#x\n ", *pressure, pressure_msb, pressure_lsb, raw_pressure);
}



/**
 * Returns the measured pressure in pascal.
 *
 * @param mpl115a2 sensor
 * @return pressure
 */
float mpl115a2_pressure(void *_s) {
	float t, p;
	mpl115a2_read_data(_s, &t, &p);
	return p;
}



/**
 * Returns the measured temperature in celsius.
 *
 * @param mpl115a2 sensor
 * @return temperature
 */
float mpl115a2_temperature(void *_s) {
	float t, p;
	mpl115a2_read_data(_s, &t, &p);
	return t;
}
