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
#include "node.hpp"
 
 /** Try to parse input to a node */
class Parser {
private:
	/** Name of the node */
	std::string _name;
	
	/** ID of the node */
	long _id;
	
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
	
	std::string name(void) const { return this->_name; }
	long id(void) const { return this->_id; }
	std::map<std::string, double> values(void) const {
		// Return copy of values
		std::map<std::string, double> ret(this->_values);
		return ret;
	}
	
	/** Extract a node out of the parsed values */
	Node node(void);
	
	void clear(void);
	
};

#endif
