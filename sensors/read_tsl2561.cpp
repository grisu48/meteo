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

#include "tsl2561.hpp"


using namespace std;
using namespace sensors;

int main() {
    TSL2561 tsl2561(TSL2561::DEFAULT_I2C_DEVICE);
    if(tsl2561.isError()) {
    	cerr << "Error opening TSL2561 sensor" << endl;
    	return EXIT_FAILURE;
    } else {
	    tsl2561.read();
	    cout << tsl2561.visible() << ", " << tsl2561.ir() << " IR" << endl;
	}
    
    return EXIT_SUCCESS;
}
