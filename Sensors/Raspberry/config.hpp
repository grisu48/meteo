/**
  * config.hpp
  * Reading a configuration file
  *
  * To provide portability of this single class, it does not use the internal string class, but implements some of it's methods new
  * The result is, that this class can be used without any other dependencies, except the STL
  *
  * Copyright 2015, Felix Niederwanger <felix@feldspaten.org>
  * MIT license (http://opensource.org/licenses/MIT)
  */

#ifndef INCLUDE_CONFIG_HPP_
#define INCLUDE_CONFIG_HPP_

#include <string>
#include <map>
#include <vector>


namespace meteo {


class Config;
class ConfigSection;

/**
 * This class reads a given configuration file and gives access to the stored values
 * The configuration file stores values in the form
 * <b>NAME = VALUE</b>
 *
 */
class Config {
private:
	/** Filename of the configuration file */
	std::string _filename;

	/** Default section, where all values without a header are written to */
	ConfigSection *defaultSection;

	/** All read values from the file */
	std::map<std::string, ConfigSection*> _sections;

	/** All illegal lines in the form LINE NUMBER -> STRING */
	std::map<int, std::string> _illegalLines;

	/** Flag indicating if the read was successfull */
	bool wasOpenend;

	/** Number of lines */
	int lines;

	/** Boolean indicating, if the keys are case sensitive or not */
	bool caseSensitive;

protected:

	/** Clear all values and release all memory */
	void clear(void);

public:
	/**
	 * New config file instance that reads from the given filename.
	 * Use the method
	 * @param filename To be opened
	 * @param caseSensitive True if the keys of the file are case sensitive
	 */
	Config(std::string filename, bool caseSensitive = true);
	/**
	 * New config file instance that reads from the given filename.
	 * Use the method
	 * @param filename To be opened
	 * @param caseSensitive True if the keys of the file are case sensitive
	 */
	Config(const char* filename, bool caseSensitive = true);
	/**
	 * Config copy constructor
	 */
	Config(Config &config);
	/**
	 * Config move constructor
	 */
	Config(Config &&config);
	/** Destructor for this config instance */
	virtual ~Config();

	/**
	 * Reads the given filename
	 * @param filename Pathname of the file to be read
	 */
	void readFile(const char* filename);

	/**
	 * @return the fileanme of the config file
	 */
	std::string filename(void);

	/**
	 * @return the number of lines in the file
	 */
	int linesCount(void);

	/**
	 * Get all illegal lines of the configuration file
	 * @param appendLineNumber if true, the result is a vector in the form LINENUMBER: LINETEXT
	 * @return vector containing all illegal lines
	 */
	std::vector<std::string> illegalLines(bool appendLineNumber = false);

	/**
	 * Checks if there are illegal lines present
	 * @return true if there is at lease one illegal line
	 */
	bool hasIllegalLines(void);

	/**
	 * Checks if there is at least one key duplicated
	 * @return true if at least one key is present more than one time
	 */
	bool hasDuplicateKeys(void);

	/**
	 * @return a vector containing all keys that are duplicate
	 */
	std::vector<std::string> duplicateKeys(void);

	/**
	 * Checks if the reading procedure was successfull
	 * @return true if the read was successfull
	 */
	bool readSuccessfull(void);


	/**
	 * Get all sections in this config file
	 * @return vector instance containing all ConfigSection instances
	 */
	std::vector<ConfigSection*> sections(void);

	/**
	 * Get the given section or NULL, if no such section exists
	 * @param key key of the section to fetch
	 * @return Pointer to the ConfigSection instance, or NULL if no such section exists
	 */
	ConfigSection* section(std::string key);

	/**
	 * Get the given section or NULL, if no such section exists
	 * @param key key of the section to fetch
	 * @return Pointer to the ConfigSection instance, or NULL if no such section exists
	 */
	ConfigSection* section(const char* key);

	/**
	 * Read the given key from the configuration file
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	std::string get(std::string key, std::string defaultValue = "", bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a integer
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	int getInt(std::string key, int defaultValue = 0, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a long
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	long getLong(std::string key, long defaultValue = 0L, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a float
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	float getFloat(std::string key, float defaultValue = 0.0F, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a double
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	double getDouble(std::string key, double defaultValue = 0.0, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a boolean
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	bool getBoolean(std::string key, bool defaultValue = false, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	std::string get(std::string key, std::string defaultValue, bool &ok);

	/**
	 * Read the given key from the configuration file and tries to parse it to a integer
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	int getInt(std::string key, int defaultValue, bool &ok);

	/**
	 * Read the given key from the configuration file and tries to parse it to a long
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	long getLong(std::string key, long defaultValue, bool &ok);

	/**
	 * Read the given key from the configuration file and tries to parse it to a float
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	float getFloat(std::string key, float defaultValue, bool &ok);

	/**
	 * Read the given key from the configuration file and tries to parse it to a double
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	double getDouble(std::string key, double defaultValue, bool &ok);


	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed double values
	 */
	std::vector<double> getDoubleValues(std::string key, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed double values
	 */
	std::vector<double> getDoubleValues(std::string key, bool &ok, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed double values
	 */
	std::vector<float> getFloatValues(std::string key, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed double values
	 */
	std::vector<float> getFloatValues(std::string key, bool &ok, std::string separator = ",");

	/**
	 * Read integer values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed integer values
	 */
	std::vector<int> getIntValues(std::string key, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read integer values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed integer values
	 */
	std::vector<int> getIntValues(std::string key, bool &ok, std::string separator = ",");

	/**
	 * Read long values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed integer values
	 */
	std::vector<long> getLongValues(std::string key, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read long values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed integer values
	 */
	std::vector<long> getLongValues(std::string key, bool &ok, std::string separator = ",");

	/**
	 * Read a value and return the separated string values
	 * @param key Key of the config name
	 * @param ok Boolean indicating if the reading of the value was true. If NULL it is ignored
	 * @param separator separator to do the splitting
	 * @return Vector of all separated strings
	 */
	std::vector<std::string> getValues(std::string key, bool *ok = NULL, std::string separator=",");

	/**
	 * Read a value and return the separated string values
	 * @param key Key of the config name
	 * @param ok Boolean indicating if the reading of the value was true. If NULL it is ignored
	 * @param separator separator to do the splitting
	 * @return Vector of all separated strings
	 */
	std::vector<std::string> getValues(std::string key, bool &ok, std::string separator=",");


	/**
	 * Read float values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, float** array, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, double** array, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read long values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, long** array, bool *ok = NULL, std::string separator = ",");


	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, int** array, bool *ok = NULL, std::string separator = ",");



	/**
	 * Read float values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, float** array, bool &ok, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, double** array, bool &ok, std::string separator = ",");

	/**
	 * Read long values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, long** array, bool &ok, std::string separator = ",");


	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, int** array, bool &ok, std::string separator = ",");

	/**
	 * Read the given key from the configuration file and tries to parse it to a boolean
	 * This call gets the key from the default section (i.e. where no section is defined)
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	bool getBoolean(std::string key, bool defaultValue, bool &ok);


	/**
	 * Read the given key from the configuration file
	 * @param key Key of the value, that should be read
	 * @return the value of the key or and empty string, if the given key doesn't exists
	 */
	std::string operator()(std::string key);

	/**
	 * @return the number of <b>name=value</b> pairs in the default section
	 */
	size_t size(void);

	/**
	 * @return All keys in the config file of the default section
	 */
	std::vector<std::string> keys(void);


	friend class ConfigSection;
};

/**
 * Section in a Config file, also containing different <b>NAME = VALUE</b> pairs
 */
class ConfigSection {

private :
	/** Parent config instance */
	Config *config;

	/** All read values from the file */
	std::map<std::string, std::string> values;

	/** Keys that are duplicate */
	std::vector<std::string> _duplicateKeys;

	/** Name of this section*/
	std::string _name;

protected:


	/** Create new ConfigSection instance for the given config and section name
	 * @param config Parent config instance
	 * @param name Name of the section or empty string, if the default section
	 * */
	ConfigSection(Config *config, std::string name);

	/** Destructor - Release memory */
	virtual ~ConfigSection();


	/** Copies the values from the given ConfigSection ref*/
	void copyFrom(ConfigSection &ref);

	/** Copies the values from the given ConfigSection ref*/
	void copyFrom(ConfigSection *ref);

	/** Copies the values from the given Config and name*/
	void copyFrom(Config &ref, std::string name);


	/**
	 * Put a key-value pair into the configuration section
	 * @param key Key of the Pair
	 * @param value Value of the pair
	 */
	void put(std::string key, std::string value);

	/** Clear contents of the section */
	void clear(void);

public:

	/**
	 * Read the given key from the configuration file
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	std::string get(std::string key, std::string defaultValue = "", bool *ok = NULL);

	/**
	 * Read the given key from the configuration file
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	std::string get(std::string key, std::string defaultValue, bool &ok);

	/**
	 * Read the given key from the configuration file and tries to parse it to a integer
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	int getInt(std::string key, int defaultValue = 0, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a integer
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	int getInt(std::string key, int defaultValue, bool &ok);

	/**
	 * Read the given key from the configuration file and tries to parse it to a long
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	long getLong(std::string key, long defaultValue = 0L, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a long
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successfull. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	long getLong(std::string key, long defaultValue, bool &ok);

	/**
	 * Read the given key from the configuration file and tries to parse it to a float
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	float getFloat(std::string key, float defaultValue = 0.0F, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a float
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	float getFloat(std::string key, float defaultValue, bool &ok);

	/**
	 * Read the given key from the configuration file and tries to parse it to a double
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	double getDouble(std::string key, double defaultValue = 0.0, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a double
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	double getDouble(std::string key, double defaultValue, bool &ok);

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed double values
	 */
	std::vector<double> getDoubleValues(std::string key, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed double values
	 */
	std::vector<double> getDoubleValues(std::string key, bool &ok, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed double values
	 */
	std::vector<float> getFloatValues(std::string key, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed double values
	 */
	std::vector<float> getFloatValues(std::string key, bool &ok, std::string separator = ",");

	/**
	 * Read integer values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed integer values
	 */
	std::vector<int> getIntValues(std::string key, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read integer values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed integer values
	 */
	std::vector<int> getIntValues(std::string key, bool &ok, std::string separator = ",");

	/**
	 * Read long values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed integer values
	 */
	std::vector<long> getLongValues(std::string key, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read long values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return vector of all parsed integer values
	 */
	std::vector<long> getLongValues(std::string key, bool &ok, std::string separator = ",");

	/**
	 * Read a value and return the separated string values
	 * @param key Key of the config name
	 * @param ok Boolean indicating if the reading of the value was true. If NULL it is ignored
	 * @param separator separator to do the splitting
	 * @return Vector of all separated strings
	 */
	std::vector<std::string> getValues(std::string key, bool *ok = NULL, std::string separator=",");

	/**
	 * Read a value and return the separated string values
	 * @param key Key of the config name
	 * @param ok Boolean indicating if the reading of the value was true. If NULL it is ignored
	 * @param separator separator to do the splitting
	 * @return Vector of all separated strings
	 */
	std::vector<std::string> getValues(std::string key, bool &ok, std::string separator=",");


	/**
	 * Read float values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, float** array, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, double** array, bool *ok = NULL, std::string separator = ",");

	/**
	 * Read long values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, long** array, bool *ok = NULL, std::string separator = ",");


	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, int** array, bool *ok = NULL, std::string separator = ",");



	/**
	 * Read float values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, float** array, bool &ok, std::string separator = ",");

	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, double** array, bool &ok, std::string separator = ",");

	/**
	 * Read long values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, long** array, bool &ok, std::string separator = ",");


	/**
	 * Read double values from the configuration file
	 * @param key Key of the value, that should be read
	 * @param array pointer to the array, that will be created
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @param separator separator to separate the values. Default value is a comma (",")
	 * @return number of elements or -1, if no elements could be read
	 */
	size_t getValues(std::string key, int** array, bool &ok, std::string separator = ",");


	/**
	 * Read the given key from the configuration file and tries to parse it to a boolean
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok Boolean set to true, if the parse is successful. If NULL, this is omitted
	 * @return the value of the key or the given default value
	 */
	bool getBoolean(std::string key, bool defaultValue = false, bool *ok = NULL);

	/**
	 * Read the given key from the configuration file and tries to parse it to a boolean
	 * @param key Key of the value, that should be read
	 * @param defaultValue Default value that is returned, if the given key doesn't exists
	 * @param ok This boolean is set to true, if the parse is successful
	 * @return the value of the key or the given default value
	 */
	bool getBoolean(std::string key, bool defaultValue, bool &ok);


	/**
	 * Read the given key from the configuration file
	 * @param key Key of the value, that should be read
	 * @return the value of the key or and empty string, if the given key doesn't exists
	 */
	std::string operator()(std::string key);

	/**
	 * @return the total number of <b>name=value</b> pairs
	 */
	size_t size(void);

	/**
	 * @return keys in this section
	 */
	std::vector<std::string> keys(void);


	/**
	 * Checks if the given key exists in the config file
	 * @param key to be checked
	 * @return true if existing, false if not
	 */
	bool hasKey(std::string key);


	/**
	 * Checks if there is at least one key duplicated
	 * @return true if at least one key is present more than one time
	 */
	bool hasDuplicateKeys(void);

	/**
	 * @return a vector containing all keys that are duplicate
	 */
	std::vector<std::string> duplicateKeys(void);


	friend class Config;
};

}



#endif /* INCLUDE_CONFIG_HPP_ */


