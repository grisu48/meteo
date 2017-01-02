/* =============================================================================
 * 
 * Title:         
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
 
#include <iostream>
#include <cstdlib>
#include <string.h>
#include <errno.h>

#include "htu21df.hpp"


using namespace std;
using namespace sensors;

int main(int argc, char** argv) {
	const char* device = HTU21DF::DEFAULT_I2C_DEVICE;
	if(argc > 1)
		device = argv[1];
		
    HTU21DF htu21df(device);
    if(htu21df.isError()) {
    	cerr << "Error opening HTU21D-F sensor: ";
    	if(errno == 0)
    		cerr << "Unknown error" << endl;
    	else 
	    	cerr << strerror(errno) << endl;
    	return EXIT_FAILURE;
    } else {
	    const int ret = htu21df.read();
	    if(ret < 0)	
	    	cerr << "Error " << errno << " while reading: " << strerror(errno) << endl;
	    cout << htu21df.temperature() << " deg C, " << htu21df.humidity() << " % rel Humidity" << endl;
	}
    
    return EXIT_SUCCESS;
}
