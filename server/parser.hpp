/* =============================================================================
 * 
 * Title:         Parser class
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2016 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   
 * 
 * =============================================================================
 */
 
#ifndef _METEO_PARSER_HPP_
#define _METEO_PARSER_HPP_

#include <string>
#include <vector>
#include <map>

#include "string.hpp"
 
class Parser {
private:
	/** Name of the node */
	std::string _node;
	
	/** Values of the node */
	std::map<std::string, double> _values;
public:
	Parser();
	~Parser();
	
	/**
	  * Tries to parse the given input
	  * @return true if parsing succeeded
	  */
	bool parse(std::string input);
	
	std::string node(void) const { return this->_node; }
	std::map<std::string, double> values(void) const {
		// Return copy of values
		std::map<std::string, double> ret(this->_values);
		return ret;
	}
	
	void clear(void);
	
};

#endif
