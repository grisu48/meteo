/* =============================================================================
 * 
 * Title:         HESSberry Monitor Header file
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
#include <unistd.h>
#include <sys/shm.h>

static const int STATE_SETUP = 1;
static const int STATE_READ = 2;
static const int STATE_OK = 0;
static const int STATE_CLOSED = 3;
static const int STATE_ERROR = 8;

struct {
	float temperature;
	float humidity;
} typedef htu21df_t;


struct {
	float broadband;
	float ir;
	float lux;
	
} typedef tsl2561_t;

// Struct for all the informations in the shared memory
struct {
	long timestamp;			// Last log timestamp
	int state;				// State. 0 if everything is fine
	
	float temperature;		// Temperature in deg C
	float humidity;			// Humidity in % rel
	
	
	float lux_broadband;	// Broadband luminosity
	float lux_visible;		// Visible luminosity
	float lux_ir;			// IR luminosity
} typedef shm_t;
