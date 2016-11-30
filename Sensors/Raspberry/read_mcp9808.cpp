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

#include "mcp9808.hpp"


using namespace std;
using namespace sensors;

int main() {
    MCP9808 mcp9808(MCP9808::DEFAULT_I2C_DEVICE);
    if(mcp9808.isError()) {
    	cerr << "Error opening MCP9808 sensor" << endl;
    	return EXIT_FAILURE;
    } else {
	    mcp9808.read();
	    cout << mcp9808.temperature() << " deg C" << endl;
	}
    
    return EXIT_SUCCESS;
}
