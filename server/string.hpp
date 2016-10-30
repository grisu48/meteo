
/* =============================================================================
 *
 * Title:         Flex' extended String class
 * Author:        Felix Niederwanger <felix@feldspaten.org>
 * License:       MIT Licence (http://opensource.org/licenses/MIT)
 * Description:   Extends the default C++ string class by some very usefull
 *                methods.
 *                Designed as subclass of std::string so that you can easily
 *                implement it in your code.
 * =============================================================================
 */

#ifndef _FLEXLIB_STRING_HPP_
#define _FLEXLIB_STRING_HPP_


#include <string>
#include <vector>


namespace flex {

/**
  * Flex' extended String class
  * Keep in mind that this class extends the std::string class and in C++ strings are immutable.
  * This could be a performance bottleneck, when having lots of string append operations
  */
class String : public std::string {

public:
	/** Initialze a new empty String */
	String() {}
	/** Initialize a String from a char array */
	String(const char* str) : std::string(str) {}
	/** Initialize a String from a char array with a given length */
	String(const char* str, size_t len) : std::string(str, len) {}
	/** Initialize a new String from a given other string instance */
	String(String &str) : std::string(str) {}
	/** Initialize a new String from a given other string instance */
	String(String *str) : std::string(*str) {}
	/** Initialize a new String from a given C++ standard string */
	String(std::string str) : std::string(str) {}

	/** Destructor */
	virtual ~String() {}

	/**
	 * Create string that is trimmed from the left
	 * @returns This string trimmed from the left as new string instance
	 */
	String ltrim(void);
	/**
	 * Create string that is trimmed from the right
	 * @returns This string trimmed from the right as new string instance
	 */
	String rtrim(void);
	/**
	 * Create string that is trimmed from both sides
	 * @returns Trimmed string as new string instance
	 */
	String trim(void);

	/**
	 * @returns This string but lowercased
	 */
	String toLowercase(void);

	/**
	 * @returns This string but uppercased
	 */
	String toUppercase(void);

	/**
	 * Search for the given string and replace it with the given other string
	 * @param searchForm Search for this string
	 * @param replaceWith Replace it with this string
	 * @returns Replaced String as a new instance
	 */
	String replace(std::string& searchFor, std::string& replaceWith);
	/**
	 * Search for the given string and replace it with the given other string
	 * @param searchForm Search for this string
	 * @param replaceWith Replace it with this string
	 * @returns Replaced String as a new instance
	 */
	String replace(const char* searchFor, const char* replaceWith);

	/**
	 * Get a substring
	 * @param pos Position from left where the read starts
	 * @param len Number of bytes to read or std::string::npos if the method should read to the end
	 * @return Substring as new String instance
	 */
	String substr(const size_t pos = 0, size_t len = std::string::npos);
	/**
	 * Read a substring from a given position to the end
	 * @return Substring as new String instance
	 */
	String mid(const size_t pos);
	/**
	 * Read a substring witch are the given amount of bytes from the left
	 * @param len Number of bytes to read from the left
	 * @return Substring as new String instance
	 */
	String left(const size_t len);
	/**
	 * Read a substring witch are the given amount of bytes from the right
	 * @param len Number of bytes to read from the right
	 * @return Substring as new String instance
	 */
	String right(const size_t len);

	/**
	 * Search for the given character from the right.
	 * @param c The character that should be searched for
	 * @returns index of the last occurrence of the given character or -1 if not found
	 */
	int lastIndexOf(const char c);
	/**
	 * Search for the given string from the right.
	 * @param str The string that should be searched for
	 * @returns index of the last occurrence of the given character or -1 if not found
	 */
	int lastIndexOf(std::string str);
	/**
	 * Search for the given character from the left.
	 * @param c The character that should be searched for
	 * @returns index of the last occurrence of the given character or -1 if not found
	 */
	int indexOf(const char c);
	/**
	 * Search for the given character from the left.
	 * @param str The string that should be searched for
	 * @returns index of the last occurrence of the given character or -1 if not found
	 */
	int indexOf(std::string str);

	/**
	 * Compare this string to another one.
	 * The comparison takes place character per character from the left.
	 * When one character is found that deviates from the string, the method returns the difference of the two characters c1, c2 in terms of (c1-c2).
	 * @retuns 0 if the two strings are the same, the difference of the first characters that alter otherwise
	 */
	int compareTo(std::string&);
	/**
	 * Compare this string to another one.
	 * The comparison takes place character per character from the left.
	 * When one character is found that deviates from the string, the method returns the difference of the two characters c1, c2 in terms of (c1-c2).
	 * @retuns 0 if the two strings are the same, the difference of the first characters that alter otherwise
	 */
	int compareTo(const char*);

	/**
	 * Compare this string to another one.
	 * The comparison takes place character per character from the left.
	 * When one character is found that deviates from the string, the method returns the difference of the two characters c1, c2 in terms of (c1-c2).
	 * The character are all lowercase before comparing
	 * @retuns 0 if the two strings are the same, the difference of the first characters that alter otherwise
	 */
	int compareToIgnoreCase(std::string&);
	/**
	 * Compare this string to another one.
	 * The comparison takes place character per character from the left.
	 * When one character is found that deviates from the string, the method returns the difference of the two characters c1, c2 in terms of (c1-c2).
	 * The character are all lowercase before comparing
	 * @retuns 0 if the two strings are the same, the difference of the first characters that alter otherwise
	 */
	int compareToIgnoreCase(const char*);

	/**
	 * Checks if this string equals to another given string
	 * @param str String to compare with
	 * @returns true if they are the same, otherwise false
	 */
	bool equals(std::string &str);
	/**
	 * Checks if this string equals to another given string by ignoring the case of the characters
	 * @param str String to compare with (but ignore the case)
	 * @returns true if they are the same, otherwise false
	 */
	bool equalsIgnoreCase(std::string &str);
	/**
	 * Checks if this string equals to another given string
	 * @param str String to compare with
	 * @returns true if they are the same, otherwise false
	 */
	bool equals(const char *str);
	/**
	 * Checks if this string equals to another given string by ignoring the case of the characters
	 * @param str String to compare with (but ignore the case)
	 * @returns true if they are the same, otherwise false
	 */
	bool equalsIgnoreCase(const char *str);

	/**
	 * Checks if this string contains another string.
	 * If this or the other string is empty the method return false
	 * @param str String to search for
	 * @returns true if the other string is found, false otherwise.
	 */
	bool contains(std::string &str);
	/**
	 * Checks if this string contains another string.
	 * If this or the other string is empty the method return false
	 * @param str String to search for
	 * @returns true if the other string is found, false otherwise.
	 */
	bool contains(const char *str);

	/**
	 * Checks if this string contains a given character
	 * If this string is empty the method return false
	 * @param c character to search for
	 * @returns true if the given parameter is found, false otherwise.
	 */
	bool contains(const char c);

	/**
	 * Checks if this string starts with a given other string
	 * @return true if this string starts with the other string
	 */
	bool startsWith(std::string &str);
	/**
	 * Checks if this string starts with a given other string
	 * @return true if this string starts with the other string
	 */
	bool startsWith(const char*);
	/**
	 * Checks if this string starts with a given character
	 * @return true if this string starts with the given character
	 */
	bool startsWith(const char);
	/**
	 * Checks if this string starts with a given other string. Ignores the case of the strings
	 * @return true if this string starts with the other string
	 */
	bool startsWithIgnoreCase(std::string&);
	/**
	 * Checks if this string starts with a given other string. Ignores the case of the strings
	 * @return true if this string ends with the other string
	 */
	bool startsWithIgnoreCase(const char*);
	/**
	 * Checks if this string ends with a given other string
	 * @return true if this string ends with the other string
	 */
	bool endsWith(std::string&);
	/**
	 * Checks if this string ends with a given other string
	 * @return true if this string ends with the other string
	 */
	bool endsWith(const char*);
	/**
	 * Checks if this string ends with a given character
	 * @return true if this string ends with the character
	 */
	bool endsWith(const char);
	/**
	 * Checks if this string ends with a given other string. Ignores the case of the strings
	 * @return true if this string ends with the other string
	 */
	bool endsWithIgnoreCase(std::string&);
	/**
	 * Checks if this string ends with a given other string. Ignores the case of the strings
	 * @return true if this string ends with the other string
	 */
	bool endsWithIgnoreCase(const char*);

	/**
	 * @returns true if this string is empty
	 */
	bool isEmpty(void);

	/**
	 * Split this string on a given separator
	 * @param separator Separator where the split should occurr
	 * @param limit Maximum number of splits or -1 if unlimited
	 * @param remoteEmpty ignore empty parameters
	 * @returns vector of the splitted String values
	 */
	std::vector<String> split(std::string& separator, size_t limit = -1, bool removeEmpty = false);

	/**
	 * Split this string on a given separator
	 * @param separator Separator where the split should occurr
	 * @param limit Maximum number of splits or -1 if unlimited
	 * @param remoteEmpty ignore empty parameters
	 * @returns vector of the splitted String values
	 */
	std::vector<String> split(const char* separator, size_t limit = -1, bool removeEmpty = false);

	/**
	 * Append an integer to this string.
	 * @returns new String instance consisting of this string and the appended integer
	 */
	String operator+(int);
	/**
	 * Append a unsigned integer to this string.
	 * @returns new String instance consisting of this string and the appended unsigned integer
	 */
	String operator+(unsigned int);

	/**
	 * Append a long to this string.
	 * @returns new String instance consisting of this string and the appended long
	 */
	String operator+(long);
	/**
	 * Append an unsigned long to this string.
	 * @returns new String instance consisting of this string and the appended unsigned long
	 */
	String operator+(unsigned long);
	/**
	 * Append a float to this string.
	 * @returns new String instance consisting of this string and the appended float
	 */
	String operator+(float);
	/**
	 * Append a double to this string.
	 * @returns new String instance consisting of this string and the appended double
	 */
	String operator+(double);
	/**
	 * Append a char to this string.
	 * @returns new String instance consisting of this string and the appended char
	 */
	String operator+(char);
	/**
	 * Append a c string to this string.
	 * @returns new String instance consisting of this string and the appended c string
	 */
	String operator+(const char*);


	/**
	 * Integer to String conversion
	 * @returns String representing the given value
	 */
	static String toString(int);
	/**
	 * Unsigned integer to String conversion
	 * @returns String representing the given value
	 */
	static String toString(unsigned int);
	/**
	 * Long to String conversion
	 * @returns String representing the given value
	 */
	static String toString(long);
	/**
	 * Unsigned long to String conversion
	 * @returns String representing the given value
	 */
	static String toString(unsigned long);
	/**
	 * Float to String conversion
	 * @returns String representing the given value
	 */
	static String toString(float);
	/**
	 * Double to String conversion
	 * @returns String representing the given value
	 */
	static String toString(double);
	/**
	 * Long double to String conversion
	 * @returns String representing the given value
	 */
	static String toString(long double);
	/**
	 * C string to String conversion
	 * @returns String representing the given value
	 */
	static String toString(const char*);
	/**
	 * C string to String conversion
	 * @returns String representing the given value
	 */
	static String toString(const char*, size_t);
	/**
	 * Character to String conversion
	 * @returns String representing the given value
	 */
	static String toString(char);


	/**
	 * Convert a string to a integer
	 * @return parsed integer
	 */
	static int toInt(std::string &str);
	/**
	 * Convert a string to a unsigned integer
	 * @return parsed unsigned integer
	 */
	static unsigned int toUInt(std::string &str);
	/**
	 * Convert a string to a long
	 * @return parsed long
	 */
	static long toLong(std::string &str);
	/**
	 * Convert a string to a unsigned long
	 * @return parsed unsigned long
	 */
	static unsigned long toULong(std::string &str);

	/**
	 * Convert a string to a float
	 * @return parsed float
	 */
	static float toFloat(std::string &str);
	/**
	 * Convert a string to a double
	 * @return parsed double
	 */
	static double toDouble(std::string &str);

	/**
	 * Convert a string to a boolean
	 * Parse values are "true", "yes", "on", "1", "yeah", "jep", "ja", "si", or "false", "no", "off", "0", "nope", "no way", "\0"
	 * The default value if the parse fails is true
	 * @return parsed boolean
	 */
	static bool toBoolean(std::string &str);

	/**
	 * @return integer parsed from this string
	 */
	int toInt(void);
	/**
	 * @return unsigned integer parsed from this string
	 */
	unsigned int toUInt(void);
	/**
	 * @return long value parsed from this string
	 */
	long toLong(void);
	/**
	 * @return unsigned long value parsed from this string
	 */
	unsigned long toULong(void);
	/**
	 * @return float parsed from this string
	 */
	float toFloat(void);
	/**
	 * @return double parsed from this string
	 */
	double toDouble(void);
	/**
	 * Converts this string to a boolean
	 * Parse values are "true", "yes", "on", "1", "yeah", "jep", "ja", "si", or "false", "no", "off", "0", "nope", "no way", "\0"
	 * The default value if the parse fails is true
	 * @return double parsed from this string
	 */
	bool toBoolean(void);
	
	/** Join the given array with the given separator.
	  * |return String with the joined array */
	static String join(std::vector<std::string> array, std::string separator = " ");
	
	/**
	 * Convert the given number to a String
	 * @return String instance from the double precision value
	 */
	static String number(double);

	/**
	 * Convert the given number to a String
	 * @return String instance from the long
	 */
	static String number(long);

	/**
	 * Convert the given number to a String
	 * @return String instance from the integer
	 */
	static String number(int);
	/**
	 * Convert the given number to a String
	 * @return String instance from the integer
	 */
	static String number(unsigned int);
	/**
	 * Convert the given number to a String
	 * @return String instance from the unsigned long
	 */
	static String number(unsigned long);
};

}


#endif
