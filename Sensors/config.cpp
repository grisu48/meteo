/* =============================================================================
 *
 * Title:         Configuration file class
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger <felix@feldspaten.org>
 *                MIT license (http://opensource.org/licenses/MIT)
 *
 * =============================================================================
 */

#include <fstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>
#include <sstream>
#include <vector>

#include <stdlib.h>

#include "config.hpp"

#define DELETE(x) { if (x!=NULL) delete x; x = NULL; }


using namespace std;

namespace meteo {


string ltrim(string input) {
	string result(input);

	result.erase(result.begin(), std::find_if(result.begin(), result.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return result;
}

string rtrim(string input) {
	string result(input);

	result.erase(std::find_if(result.rbegin(), result.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), result.end());
	return result;
}

string trim(string input) {
	string result(input);
	result.erase(result.begin(), std::find_if(result.begin(), result.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	result.erase(std::find_if(result.rbegin(), result.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), result.end());
	return result;
}

string lowercase(string input) {
	string result(input);
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}




Config::Config(std::string filename, bool caseSensitive) {
	this->_filename = filename;
	this->wasOpenend = false;
	this->lines = 0;
	this->caseSensitive = caseSensitive;
	this->defaultSection = NULL;		// Will be created in readFile()

	this->readFile(filename.c_str());
}

Config::Config(const char* filename, bool caseSensitive) {
	this->_filename = string(filename);
	this->wasOpenend = false;
	this->lines = 0;
	this->caseSensitive = caseSensitive;
	this->defaultSection = NULL;		// Will be created in readFile()

	this->readFile(filename);
}

Config::Config(Config &config) : Config(config._filename, config.caseSensitive) {}

Config::Config(Config &&config) {
	this->_filename = config._filename;
	this->wasOpenend = config.wasOpenend;
	this->lines = config.lines;
	this->caseSensitive = config.caseSensitive;
	this->defaultSection = config.defaultSection;
	
	// Dispose old config
	config.defaultSection = NULL;
	for(map<string, ConfigSection*>::iterator it = config._sections.begin(); it != config._sections.end(); ++it) {
		this->_sections[it->first] = it->second;
	}
	config._sections.clear();
}

Config::~Config() {
	this->clear();
}

void Config::clear(void) {
	// Dispose default section
	DELETE(this->defaultSection);
	
	// Delete all configsections
	for(map<string, ConfigSection*>::iterator it = this->_sections.begin(); it != this->_sections.end(); ++it) {
		if(it->second == this->defaultSection) continue;
		else delete it->second;
	}
	this->_sections.clear();
}



void Config::readFile(const char* filename) {
	this->clear();
	this->defaultSection = new ConfigSection(this, "");
	this->wasOpenend = false;
	this->_filename = string(filename);


	ifstream in(filename);
	if(!in.is_open())
		return;

	string line;
	string sectionName = "";
	ConfigSection *section = this->defaultSection;
	int lineNo = 0;
	while((getline(in, line))) {
		lineNo++;
		line = ltrim(line);
		if (line.length() == 0) continue;



		try {
			// Check if the line is a comment
			const char firstChar = line.at(0);
			if(firstChar == '#' || firstChar == ':' || firstChar == ';' || firstChar == '-' || firstChar == '=')
				continue;

			// Check if new section
			if(firstChar == '[') {
				// Check for complete header (begins with '[' and ends with ']') and if the header is not empty
				const char lastChar = line.at(line.length()-1);
				if(lastChar != ']')
					throw "Incomplete section header";
				if (line == "[]")
					throw "Empty section header";

				// Get name
				sectionName = trim(line.substr(1, line.length()-2));
				if(sectionName.length()==0) throw "Empty section name";

				// Add a new config section
				section = new ConfigSection(this, sectionName);
				string key = sectionName;
				if(!caseSensitive) key = lowercase(key);
				this->_sections[key] = section;

			} else {
				// Check for separator
				size_t separator = line.find('=');
				if(separator == string::npos) {
					throw "Missing separator";
				}

				// Extract name and value
				string name = trim(line.substr(0,separator-1));
				if(!caseSensitive) name = lowercase(name);
				string value = trim(line.substr(separator+1));
				if(name.length() == 0) {
					// Empty name, consider it as illegal line
					throw "Empty name";
				}

				// Add to values map
				section->put(name, value);
			}
		} catch (...) {
			// Is an illegal line
			this->_illegalLines[lineNo] = line;
			continue;
		}
	}
	this->lines = lineNo;

	in.close();
	this->wasOpenend = true;
}


string Config::filename(void) {
	return this->_filename;
}

bool Config::readSuccessfull(void) {
	return this->wasOpenend;
}

vector<string> Config::illegalLines(bool appendLineNumber) {
	vector<string> result;
	if(this->_illegalLines.size() == 0) return result;

	// Fill line buffer
	if(appendLineNumber) {
		// Expected result in the form LINENO: LINETEXT

		stringstream buf;
		for(map<int,string>::iterator it = this->_illegalLines.begin(); it != this->_illegalLines.end(); ++it) {
			buf.str("");
			buf << it->first << ": " << it->second;
			result.push_back(buf.str());
		}
	} else {
		// Just add the lines
		for(map<int,string>::iterator it = this->_illegalLines.begin(); it != this->_illegalLines.end(); ++it)
			result.push_back(it->second);
	}

	return result;
}

bool Config::hasIllegalLines(void) {
	return this->_illegalLines.size() > 0;
}

bool Config::hasDuplicateKeys(void) {
	return defaultSection->hasDuplicateKeys();
}

vector<string> Config::duplicateKeys(void) {
	return defaultSection->duplicateKeys();
}


vector<ConfigSection*> Config::sections(void) {
	// Return copy of the list to prevent that the user can damage the list structure or (even worse)
	// some ConfigSection references are lost and we have a memory leak
	vector<ConfigSection*> result;
	for(map<string, ConfigSection*>::iterator it = this->_sections.begin(); it != this->_sections.end(); ++it)
		result.push_back(it->second);

	return result;
}


ConfigSection* Config::section(string key) {
	key = trim(key);
	if(key.length() == 0) return this->defaultSection;
	if(!caseSensitive) key = lowercase(key);

	if (this->_sections.find(key) == this->_sections.end()) return NULL;
	else
		return this->_sections[key];
}

ConfigSection* Config::section(const char* key) {
	return this->section(std::string(key));
}

std::string Config::get(string key, string defaultValue, bool *ok) {
	return this->defaultSection->get(key, defaultValue, ok);
}

int Config::getInt(string key, int defaultValue, bool *ok) {
	return this->defaultSection->getInt(key, defaultValue, ok);
}

long Config::getLong(string key, long defaultValue, bool *ok) {
	return this->defaultSection->getLong(key, defaultValue, ok);
}

float Config::getFloat(string key, float defaultValue, bool *ok) {
	return this->defaultSection->getFloat(key, defaultValue, ok);
}

double Config::getDouble(string key, double defaultValue, bool *ok) {
	return this->defaultSection->getDouble(key, defaultValue, ok);
}

bool Config::getBoolean(std::string key, bool defaultValue, bool *ok) {
	return defaultSection->getBoolean(key, defaultValue, ok);
}

bool Config::getBoolean(std::string key, bool defaultValue, bool &ok) {
	return this->getBoolean(key, defaultValue, &ok);
}

double Config::getDouble(std::string key, double defaultValue, bool &ok) {
	return this->getDouble(key, defaultValue, &ok);
}

float Config::getFloat(std::string key, float defaultValue, bool &ok) {
	return this->getFloat(key, defaultValue, &ok);
}

long Config::getLong(std::string key, long defaultValue, bool &ok) {
	return this->getLong(key, defaultValue, &ok);
}

std::string Config::get(std::string key, std::string defaultValue, bool &ok) {
	return this->get(key, defaultValue, &ok);
}

int Config::getInt(std::string key, int defaultValue, bool &ok) {
	return this->getInt(key, defaultValue, &ok);
}

string Config::operator()(string key) {
	return this->defaultSection->get(key, "", NULL);
}

size_t Config::size(void) {
	return this->defaultSection->size();
}


std::vector<double> Config::getDoubleValues(std::string key, bool *ok, std::string separator) {
	return this->defaultSection->getDoubleValues(key, ok, separator);
}

std::vector<double> Config::getDoubleValues(std::string key, bool &ok, std::string separator) {
	return this->defaultSection->getDoubleValues(key, ok, separator);
}

std::vector<float> Config::getFloatValues(std::string key, bool *ok, std::string separator) {
	return this->defaultSection->getFloatValues(key, ok, separator);
}

std::vector<float> Config::getFloatValues(std::string key, bool &ok, std::string separator) {
	return this->defaultSection->getFloatValues(key, ok, separator);
}

std::vector<int> Config::getIntValues(std::string key, bool *ok, std::string separator) {
	return this->defaultSection->getIntValues(key, ok, separator);
}

std::vector<int> Config::getIntValues(std::string key, bool &ok, std::string separator) {
	return this->defaultSection->getIntValues(key, ok, separator);
}

std::vector<long> Config::getLongValues(std::string key, bool *ok, std::string separator) {
	return this->defaultSection->getLongValues(key, ok, separator);
}

std::vector<long> Config::getLongValues(std::string key, bool &ok, std::string separator) {
	return this->defaultSection->getLongValues(key, ok, separator);
}

std::vector<std::string> Config::getValues(std::string key, bool *ok, std::string separator) {
	return this->defaultSection->getValues(key, ok, separator);
}

std::vector<std::string> Config::getValues(std::string key, bool &ok, std::string separator) {
	return this->defaultSection->getValues(key, ok, separator);
}

size_t Config::getValues(std::string key, float** array, bool *ok, std::string separator) {
	return this->defaultSection->getValues(key, array, ok, separator);
}

size_t Config::getValues(std::string key, double** array, bool *ok, std::string separator) {
	return this->defaultSection->getValues(key, array, ok, separator);
}

size_t Config::getValues(std::string key, long** array, bool *ok, std::string separator) {
	return this->defaultSection->getValues(key, array, ok, separator);
}

size_t Config::getValues(std::string key, int** array, bool *ok, std::string separator) {
	return this->defaultSection->getValues(key, array, ok, separator);
}

size_t Config::getValues(std::string key, float** array, bool &ok, std::string separator) {
	return this->defaultSection->getValues(key, array, ok, separator);
}

size_t Config::getValues(std::string key, double** array, bool &ok, std::string separator) {
	return this->defaultSection->getValues(key, array, ok, separator);
}

size_t Config::getValues(std::string key, long** array, bool &ok, std::string separator) {
	return this->defaultSection->getValues(key, array, ok, separator);
}

size_t Config::getValues(std::string key, int** array, bool &ok, std::string separator) {
	return this->defaultSection->getValues(key, array, ok, separator);
}









ConfigSection::ConfigSection(Config *ref, std::string name) : config(ref) {
	this->_name = name;
}

void ConfigSection::copyFrom(ConfigSection &ref) {
	this->copyFrom(&ref);
}

void ConfigSection::copyFrom(ConfigSection *ref) {
	if(ref == NULL) throw "Null reference";
	this->_name = ref->_name;
	this->config = ref->config;


	// Copy all values
	this->values.clear();
	for(map<string,string>::iterator it = ref->values.begin(); it != ref->values.end(); it++)
		this->values[it->first] = it->second;
	this->_duplicateKeys.clear();
	for(vector<string>::iterator it = ref->_duplicateKeys.begin(); it != ref->_duplicateKeys.end(); ++it) {
		this->_duplicateKeys.push_back(*it);
	}
}

void ConfigSection::copyFrom(Config &ref, std::string name) {
	ConfigSection *section = ref.section(name);
	if(section == NULL) {
		// Section not found
		this->_name = name;
	} else
		this->copyFrom(section);
}

ConfigSection::~ConfigSection() {

}

vector<string> ConfigSection::duplicateKeys(void) {
	vector<string> result;
	for(vector<string>::iterator it = this->_duplicateKeys.begin(); it != this->_duplicateKeys.end(); ++it)
		result.push_back(*it);
	return result;
}

bool ConfigSection::hasDuplicateKeys(void) {
	return this->_duplicateKeys.size() > 0;
}

void ConfigSection::put(std::string key, std::string value) {
	key = trim(key);
	if(key.length() == 0) return;
	else {
		if(!this->config->caseSensitive) key = lowercase(key);

		// Check if duplicate key
		if(this->hasKey(key))
			this->_duplicateKeys.push_back(key);

		this->values[key] = value;
	}
}

size_t ConfigSection::size(void) {
	return this->values.size();
}

bool ConfigSection::hasKey(std::string key) {
	key = trim(key);
	if(key.length() == 0) return false;

	if(!this->config->caseSensitive) key = lowercase(key);
	return this->values.find(key) != this->values.end();
}

std::string ConfigSection::get(string key, string defaultValue, bool *ok) {
	if(!this->config->caseSensitive) key = lowercase(key);
	if(!hasKey(key)) {
		if(ok != NULL) *ok = false;
		return defaultValue;
	} else {
		if(ok != NULL) *ok = true;
		return this->values[key];
	}
}

int ConfigSection::getInt(string key, int defaultValue, bool *ok) {
	return (int)getLong(key, defaultValue, ok);
}

long ConfigSection::getLong(string key, long defaultValue, bool *ok) {
	if(!hasKey(key)) {
		if(ok != NULL) *ok = false;
		return defaultValue;
	} else {
		string value = this->values[key];

		// Parse string to long
		char* endptr = NULL;
		long result = strtol(value.c_str(), &endptr, 10);
		if(endptr == NULL) {
			if(ok != NULL) *ok = false;
			return defaultValue;
		}

		if(ok != NULL) *ok = true;
		return result;

	}
}

float ConfigSection::getFloat(string key, float defaultValue, bool *ok) {
	return (float)getDouble(key, defaultValue, ok);
}

double ConfigSection::getDouble(string key, double defaultValue, bool *ok) {
	if(!hasKey(key)) {
		if(ok != NULL) *ok = false;
		return defaultValue;
	} else {
		string value = this->values[key];

		// Parse string to long
		char* endptr = NULL;
		double result = strtod(value.c_str(), &endptr);
		//if(endptr == NULL) return defaultValue;
		if(!endptr[0] == '\0') {
			if(ok != NULL) *ok = false;
			return defaultValue;
		}
		if(ok != NULL) *ok = true;
		return result;

	}
}

bool ConfigSection::getBoolean(std::string key, bool defaultValue, bool *ok) {
	string value = this->get(key, defaultValue?"true":"false");
	value = lowercase(trim(value));
	if(value.length() == 0) {
		if(ok != NULL) *ok = false;
		return defaultValue;
	}

	if(value == "true" || value == "1" || value == "on" || value == "yes") {
		if(ok != NULL) *ok = true;
		return true;
	}
	if(value == "false" || value == "0" || value == "off" || value == "no") {
		if(ok != NULL) *ok = true;
		return false;
	}

	if(ok != NULL) *ok = false;
	return defaultValue;
}

bool ConfigSection::getBoolean(std::string key, bool defaultValue, bool &ok) {
	return this->getBoolean(key, defaultValue, &ok);
}

double ConfigSection::getDouble(std::string key, double defaultValue, bool &ok) {
	return this->getDouble(key, defaultValue, &ok);
}

float ConfigSection::getFloat(std::string key, float defaultValue, bool &ok) {
	return this->getFloat(key, defaultValue, &ok);
}

long ConfigSection::getLong(std::string key, long defaultValue, bool &ok) {
	return this->getLong(key, defaultValue, &ok);
}

std::string ConfigSection::get(std::string key, std::string defaultValue, bool &ok) {
	return this->get(key, defaultValue, &ok);
}

int ConfigSection::getInt(std::string key, int defaultValue, bool &ok) {
	return this->getInt(key, defaultValue, &ok);
}

vector<string> ConfigSection::keys(void) {
	vector<string> result;
	for(map<string,string>::iterator it = this->values.begin(); it != this->values.end(); ++it)
		result.push_back(it->first);
	return result;
}


vector<string> ConfigSection::getValues(string key, bool *ok, string separator) {
	bool tmp_bool;
	if(ok == NULL) ok = &tmp_bool;
	string value = this->get(key, "", ok);
	vector<string> result;
	if(!*ok) return result;		// Not good

	// Parse
	while(value.length() > 0) {
		string token;
		const size_t index = value.find(separator);
		if(index == string::npos) {
			token = trim(value);
			result.push_back(token);
			break;
		} else {
			token = trim(value.substr(0,index));
			value = ltrim(value.substr(index+1));
			result.push_back(token);
		}
	}

	// All good
	*ok = true;
	return result;
}

vector<string> ConfigSection::getValues(string key, bool &ok, string separator) {
	return this->getValues(key, &ok, separator);
}



vector<double> ConfigSection::getDoubleValues(std::string key, bool &ok, std::string separator) {
	return this->getDoubleValues(key, &ok, separator);
}

vector<double> ConfigSection::getDoubleValues(std::string key, bool *ok, std::string separator) {
	vector<string> tokens = this->getValues(key, ok, separator);
	vector<double> result;
	if(!*ok) return result;		// Not good

	for(vector<string>::iterator it=tokens.begin(); it!=tokens.end(); ++it) {
		string token = *it;
		result.push_back(atof(token.c_str()));
	}

	return result;
}

std::vector<int> ConfigSection::getIntValues(std::string key, bool *ok, std::string separator) {
	vector<string> tokens = this->getValues(key, ok, separator);
	vector<int> result;
	if(!*ok) return result;		// Not good

	for(vector<string>::iterator it=tokens.begin(); it!=tokens.end(); ++it) {
		string token = *it;
		result.push_back((int)atol(token.c_str()));
	}

	return result;
}

std::vector<int> ConfigSection::getIntValues(std::string key, bool &ok, std::string separator) {
	return this->getIntValues(key, &ok, separator);
}

std::vector<float> ConfigSection::getFloatValues(std::string key, bool *ok, std::string separator) {
	vector<string> tokens = this->getValues(key, ok, separator);
	vector<float> result;
	if(!*ok) return result;		// Not good

	for(vector<string>::iterator it=tokens.begin(); it!=tokens.end(); ++it) {
		string token = *it;
		result.push_back((float)atof(token.c_str()));
	}

	return result;
}

std::vector<float> ConfigSection::getFloatValues(std::string key, bool &ok, std::string separator) {
	return this->getFloatValues(key, &ok, separator);
}

vector<long> ConfigSection::getLongValues(string key, bool *ok, std::string separator) {
	vector<string> tokens = this->getValues(key, ok, separator);
	vector<long> result;
	if(!*ok) return result;		// Not good

	for(vector<string>::iterator it=tokens.begin(); it!=tokens.end(); ++it) {
		string token = *it;
		result.push_back(atol(token.c_str()));
	}

	return result;
}

vector<long> ConfigSection::getLongValues(string key, bool &ok, std::string separator) {
	return this->getLongValues(key, &ok, separator);
}






size_t ConfigSection::getValues(std::string key, double** array, bool *ok, std::string separator) {
	bool temp_ok;
	if(ok == NULL) ok = &temp_ok;
	vector<double> values = this->getDoubleValues(key, ok, separator);
	if(!ok) return -1;

	// Create array
	const size_t n = values.size();
	double* buf = new double[n];
	for(size_t i=0;i<n;i++) buf[i] = values[i];
	*array = buf;
	return n;
}

size_t ConfigSection::getValues(std::string key, int** array, bool *ok, std::string separator) {
	bool temp_ok;
	if(ok == NULL) ok = &temp_ok;
	vector<int> values = this->getIntValues(key, ok, separator);
	if(!ok) return -1;

	// Create array
	const size_t n = values.size();
	int* buf = new int[n];
	for(size_t i=0;i<n;i++) buf[i] = values[i];
	*array = buf;
	return n;
}

size_t ConfigSection::getValues(std::string key, float** array, bool *ok, std::string separator) {
	bool temp_ok;
	if(ok == NULL) ok = &temp_ok;
	vector<float> values = this->getFloatValues(key, ok, separator);
	if(!ok) return -1;

	// Create array
	const size_t n = values.size();
	float* buf = new float[n];
	for(size_t i=0;i<n;i++) buf[i] = values[i];
	*array = buf;
	return n;
}

size_t ConfigSection::getValues(std::string key, long** array, bool *ok, std::string separator) {
	bool temp_ok;
	if(ok == NULL) ok = &temp_ok;
	vector<long> values = this->getLongValues(key, ok, separator);
	if(!ok) return -1;

	// Create array
	const size_t n = values.size();
	long* buf = new long[n];
	for(size_t i=0;i<n;i++) buf[i] = values[i];
	*array = buf;
	return n;
}

size_t ConfigSection::getValues(std::string key, float** array, bool &ok, std::string separator) {
	return this->getValues(key, array, &ok, separator);
}

size_t ConfigSection::getValues(std::string key, double** array, bool &ok, std::string separator) {
	return this->getValues(key, array, &ok, separator);
}

size_t ConfigSection::getValues(std::string key, long** array, bool &ok, std::string separator) {
	return this->getValues(key, array, &ok, separator);
}

size_t ConfigSection::getValues(std::string key, int** array, bool &ok, std::string separator) {
	return this->getValues(key, array, &ok, separator);
}

void ConfigSection::clear(void) {
	this->values.clear();
	this->_duplicateKeys.clear();
}


}


