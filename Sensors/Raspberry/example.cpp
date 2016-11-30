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

// Maximum Library. See https://github.com/grisu48/FlexLib
// Note: Compile with -pthread -lhdf5 -libflex -lm lz -lsqlite3 `mysql_config --cflags --libs`
#include "bmp180.hpp"


using namespace std;
using namespace sensors;

int main() {
    cout << "LibMeteo - Sensors example program" << endl;
    
    BMP180 bmp180("/dev/i2c-1");
    bmp180.read();
    cout << bmp180.temperature() << " deg C, " << bmp180.pressure() << " hPa" << endl;
    
    return EXIT_SUCCESS;
}
