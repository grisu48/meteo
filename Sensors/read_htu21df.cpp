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

#include "htu21df.hpp"


using namespace std;
using namespace sensors;

int main() {
    HTU21DF htu21df(HTU21DF::DEFAULT_I2C_DEVICE);
    if(htu21df.isError()) {
    	cerr << "Error opening HTU21D-F sensor" << endl;
    	return EXIT_FAILURE;
    } else {
	    htu21df.read();
	    cout << htu21df.temperature() << " deg C, " << htu21df.humidity() << " % rel Humidity" << endl;
	}
    
    return EXIT_SUCCESS;
}
