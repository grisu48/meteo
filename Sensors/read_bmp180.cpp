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

#include "bmp180.hpp"


using namespace std;
using namespace sensors;

int main() {
//    cout << "LibMeteo - Read BMP180 Sensor" << endl;
    
    BMP180 bmp180(BMP180::DEFAULT_I2C_DEVICE);
    if(bmp180.isError()) {
    	cerr << "Error opening BMP180 sensor" << endl;
    	return EXIT_FAILURE;
    } else {
	    bmp180.read();
	    cout << bmp180.temperature() << " deg C, " << bmp180.pressure() << " hPa" << endl;
	}
    
    return EXIT_SUCCESS;
}
