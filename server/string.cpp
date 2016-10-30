
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

#include <cstring>
#include <math.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>
#include <vector>

#include "string.hpp"


using namespace std;

namespace flex {

// TRUE values for the boolean conversion. The first value is used as reference for the conversion from bool to a string
// Note that "\0" marks the end of the array
static String TRUE_VALUES[] = {"true", "yes", "on", "1", "yeah", "jep", "ja", "si", "\0" };
// FALSE values for the boolean conversion. The first value is used as reference for the conversion from bool to a string
// Note that "\0" marks the end of the array
static String FALSE_VALUES[] = {"false", "no", "off", "0", "nope", "no way", "\0"};


#if __cplusplus < 201103L

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

string to_string(int value) { return SSTR(value); }
string to_string(unsigned int value) { return SSTR(value); }
string to_string(float value) { return SSTR(value); }
string to_string(long value) { return SSTR(value); }
string to_string(unsigned long value) { return SSTR(value); }
string to_string(double value) { return SSTR(value); }

#else

#define SSTR(x) ::to_string(x)

#endif

String String::ltrim() {
	String result(this);

	result.erase(result.begin(), std::find_if(result.begin(), result.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return result;
}

String String::rtrim() {
	String result(this);

	result.erase(std::find_if(result.rbegin(), result.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), result.end());
	return result;
}
String String::trim() {
	String result(this);
	result.erase(result.begin(), std::find_if(result.begin(), result.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	result.erase(std::find_if(result.rbegin(), result.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), result.end());

	return result;
}
String String::toLowercase() {
	String result(this);
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}
String String::toUppercase() {
	String result(this);
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

String String::replace(std::string& searchFor, std::string& replaceWith) {
	string result(this->c_str());
	/*size_t start_pos;

	while( (start_pos = result.find(searchFor)) != string::npos)
		((std::string)result).replace(start_pos, searchFor.length(), replaceWith);

	return result;*/

    size_t start_pos = 0;
    while((start_pos = result.find(searchFor, start_pos)) != std::string::npos) {
        result.replace(start_pos, searchFor.length(), replaceWith);
        start_pos += replaceWith.length();
    }
    return String(result);
}

String String::replace(const char* searchFor, const char* replaceWith) {
	string str1 = string(searchFor);
	string str2 = string(replaceWith);
	return this->replace(str1, str2);
}

String String::substr(const size_t pos, size_t len) {
	return String(std::string::substr(pos, len));
}

String String::left(const size_t len) {
	size_t size = this->size();
	if(len>=size) return String("");
	else return substr(0,len);
}

String String::right(const size_t len) {
	size_t size = this->size();
	if(size<len) return String(this);
	else {
		size_t pos = size-len;
		return substr(pos);
	}
}



int String::compareTo(const char* str) {
	std::string str_instance = std::string(str);
	return this->compareTo(str_instance);
}

int String::compareToIgnoreCase(const char* str) {
	std::string str_instance = std::string(str);
	return this->compareToIgnoreCase(str_instance);
}

int String::compareTo(std::string &str) {
	const size_t thisSize = this->size();
	const size_t otherSize = str.size();

	const size_t len = min(thisSize, otherSize);
	for(size_t i = 0;i<len;i++) {
		char c1 = std::string::at(i);
		char c2 = str.at(i);
		if(c1!=c2) return c1-c2;
	}
	return thisSize - otherSize;
}

int String::compareToIgnoreCase(std::string &str) {
	const size_t thisSize = this->size();
	const size_t otherSize = str.size();

	const size_t len = min(thisSize, otherSize);
	for(size_t i = 0;i<len;i++) {
		char c1 = ::tolower(std::string::at(i));
		char c2 = ::tolower(str.at(i));
		if(c1!=c2) {
			c1 = ::toupper(std::string::at(i));
			c2 = ::toupper(str.at(i));
			if(c1!=c2) return c1-c2;
		}
	}
	return thisSize - otherSize;
}

bool String::contains(std::string &str) {
	const size_t thisSize = this->size();
	const size_t otherSize = str.size();

	if(thisSize > otherSize) return false;
	if(thisSize == 0 || otherSize == 0) return false;
	return (std::string::find(str) != std::string::npos);
}

bool String::contains(const char* str) {
	const size_t thisSize = this->size();
	const size_t otherSize = strlen(str);

	if(thisSize > otherSize) return false;
	if(thisSize == 0 || otherSize == 0) return false;
	return (std::string::find(str) != std::string::npos);
}

bool String::contains(const char c) {
	if(this->size() < 1) return false;
	return (std::string::find(c) != std::string::npos);
}


bool String::startsWith(const char* str) {
	std::string str_instance = std::string(str);
	return this->startsWith(str_instance);
}
bool String::startsWithIgnoreCase(const char* str) {
	std::string str_instance = std::string(str);
	return this->startsWithIgnoreCase(str_instance);
}
bool String::startsWith(const char c) {
	if(isEmpty()) return false;
	return this->at(0) == c;
}

bool String::endsWith(const char* str) {
	std::string str_instance = std::string(str);
	return this->endsWith(str_instance);
}
bool String::endsWithIgnoreCase(const char* str) {
	std::string str_instance = std::string(str);
	return this->endsWithIgnoreCase(str_instance);
}
bool String::endsWith(const char c) {
	const size_t size = this->size();
	if(size == 0) return false;
	return this->at(size-1) == c;
}

bool String::startsWith(std::string &str) {
	const size_t thisSize = this->size();
	const size_t otherSize = str.size();

	if(thisSize < otherSize) return false;
	if(thisSize == 0 || otherSize == 0) return false;
	return std::string::substr(0, otherSize) == str;
}

bool String::startsWithIgnoreCase(std::string &str) {
	const size_t thisSize = this->size();
	const size_t otherSize = str.size();

	if(thisSize < otherSize) return false;
	if(thisSize == 0 || otherSize == 0) return false;
	String strLowercase = String(str).toLowercase();
	String thisLowercase = this->toLowercase();
	return thisLowercase.startsWith(strLowercase);
}

bool String::endsWith(std::string &str) {
	const size_t thisSize = this->size();
	const size_t otherSize = str.size();

	if(thisSize < otherSize) return false;
	if(thisSize == 0 || otherSize == 0) return false;
	return std::string::substr(thisSize-otherSize, otherSize) == str;

}
bool String::endsWithIgnoreCase(std::string &str)  {
	const size_t thisSize = this->size();
	const size_t otherSize = str.size();

	if(thisSize < otherSize) return false;
	if(thisSize == 0 || otherSize == 0) return false;
	String strLowercase = String(str).toLowercase();
	String thisLowercase = this->toLowercase();
	return thisLowercase.endsWith(strLowercase);
}

bool String::isEmpty(void) {
	return this->length() == 0;
}

std::vector<String> String::split(const char* separator, size_t limit, bool removeEmpty) {
	string sep_str(separator);
	return this->split(sep_str,limit,removeEmpty);
}

std::vector<String> String::split(std::string& separator, size_t limit, bool removeEmpty) {
	vector<String> result;
	String remaining = String(this);
	if(separator.length() <= 0) {
		result.push_back(remaining);
		return result;
	}

	while(true) {
		size_t index = remaining.find(separator);
		if(index == std::string::npos) break;

		String item = remaining.substr(0, index);
		remaining = remaining.substr(index+1);
		if(removeEmpty && item.size() == 0) continue;
		result.push_back(item);
		if(limit > 0 && result.size() >= (size_t)limit-1) break;
	}
	if(remaining.size() > 0)
		result.push_back(remaining);

	return result;
}

bool String::equals(const char* msg) {
	std::string str(msg);
	return this->equals(str);
}
bool String::equalsIgnoreCase(const char* msg) {
	std::string str(msg);
	return this->equalsIgnoreCase(str);
}

bool String::equals(std::string &str) {
	return this->compareTo(str) == 0;
}
bool String::equalsIgnoreCase(std::string &str) {
	return this->compareToIgnoreCase(str) == 0;
}

String String::mid(size_t pos) {
	return this->substr(pos);
}

int String::lastIndexOf(const char c) {
	const size_t index = this->rfind(c);
	if(index == std::string::npos) return -1;
	else return index;
}

int String::lastIndexOf(std::string str) {
	const size_t index = this->rfind(str);
	if(index == std::string::npos) return -1;
	else return index;
}

int String::indexOf(const char c) {
	const size_t index = this->find(c);
	if(index == std::string::npos) return -1;
	else return index;
}
int String::indexOf(std::string str) {
	const size_t index = this->find(str);
	if(index == std::string::npos) return -1;
	else return index;
}


String String::operator+(int value) {
	String result = String(this);
	result += to_string(value);
	return result;
}

String String::operator+(unsigned int value) {
	String result = String(this);
	result += to_string(value);
	return result;
}

String String::operator+(long value) {
	String result = String(this);
	result += to_string(value);
	return result;
}

String String::operator+(unsigned long value) {
	String result = String(this);
	result += to_string(value);
	return result;
}

String String::operator+(float value) {
	String result = String(this);
	result += to_string(value);
	return result;
}

String String::operator+(double value) {
	String result = String(this);
	result += to_string(value);
	return result;
}

String String::operator+(char value) {
	String result = String(this);
	result += to_string(value);
	return result;
}

String String::operator+(const char* value) {
	String result = String(this);
	result += String(value);
	return result;
}


String String::toString(int i) { return SSTR(i); }
String String::toString(unsigned int i) { return SSTR(i); }
String String::toString(long l)  { return SSTR(l); }
String String::toString(unsigned long l)  { return SSTR(l); }
String String::toString(float f)  { return SSTR(f); }
String String::toString(double d)  { return SSTR(d); }
String String::toString(long double ld)  { return SSTR(ld); }
String String::toString(const char* c) {
	std::string str(c);
	return String(str);
}
String String::toString(const char* c, size_t len) {
	std::string str(c, len);
	return String(str);
}
String String::toString(char c) {
	String result;
	result += c;
	return result;
}


int String::toInt(std::string &str) {
	if (str.length() == 0) return 0;
	else
		return atoi(str.c_str());
}

unsigned int String::toUInt(std::string &str) {
	return (unsigned int)(String::toInt(str));
}

long String::toLong(std::string &str) {
	if(str.length() == 0) return 0L;
	else
		return atol(str.c_str());
}

unsigned long String::toULong(std::string &str) {
	return (unsigned long)(String::toLong(str));
}

float String::toFloat(std::string &str) {
	return (float)String::toDouble(str);
}

double String::toDouble(std::string &str) {
	if(str.length() == 0) return 0.0;
	else
		return atof(str.c_str());
}

bool String::toBoolean(std::string &str) {
	if(str.length() == 0) return 0.0;
	String compare = String(str);
	compare = compare.trim().toLowercase();

	// Check for TRUE and FALSE values
	for(int i=0; TRUE_VALUES[i] != "\0"; i++)
		if (compare == TRUE_VALUES[i].trim().toLowercase()) return true;

	for(int i=0; FALSE_VALUES[i] != "\0"; i++)
		if (compare == FALSE_VALUES[i].trim().toLowercase()) return false;

	// Default value
	return false;
}


int String::toInt(void) { return String::toInt(*this); }
unsigned int String::toUInt(void) { return String::toUInt(*this); }
long String::toLong(void) { return String::toLong(*this); }
unsigned long String::toULong(void) { return String::toULong(*this); }
float String::toFloat(void) { return String::toFloat(*this); }
double String::toDouble(void) { return String::toDouble(*this); }
bool String::toBoolean(void) { return String::toBoolean(*this); }

String String::number(double x) {
	std::string str = to_string(x);
	return String(str);
}

String String::number(int x) {
	std::string str = to_string(x);
	return String(str);
}

String String::number(unsigned int x) {
	std::string str = to_string(x);
	return String(str);
}

String String::number(long x) {
	std::string str = to_string(x);
	return String(str);
}
String String::number(unsigned long x) {
	std::string str = to_string(x);
	return String(str);
}

String String::join(vector<string> array, std::string separator) {
	stringstream ss;
	
	bool first = true;
	for(vector<string>::iterator it = array.begin(); it != array.end(); ++it) {
		if(first) first = false;
		else ss << separator;
		ss << *it;
	}
	
	return String(ss.str());
}

}
