/* =============================================================================
 *
 * Title:         Time functions. This header file is meant as a standalone
 * Author:        Felix Niederwanger, 2017 <felix@feldspaten.org>
 * License:       GPLv3 (https://www.gnu.org/licenses/gpl-3.0.en.html)
 *
 * =============================================================================
 */




#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include "time.hpp"


using namespace std;

namespace lazy {


int64_t getSystemTime(void) {
	struct timeval tv;
	int64_t result;
	gettimeofday(&tv, 0);
	result = tv.tv_sec;
	result *= 1000L;
	result += (int64_t)tv.tv_usec/1000L;
	return result;
}


int64_t getSystemTimeMicros(void) {
	struct timeval tv;
	int64_t result;
	gettimeofday(&tv, 0);
	result = tv.tv_sec;
	result *= 1000L*1000L;
	result += (int64_t)tv.tv_usec;
	return result;
}

void Sleep(long millis, long micros) {
	struct timespec tt, rem;
	int ret;

	tt.tv_sec = millis/1000L;
	millis -= tt.tv_sec*1000L;
	tt.tv_nsec = (micros + millis*1000L)*1000L;
	ret = nanosleep(&tt, &rem);
	if(ret < 0) {
		switch(errno) {
		case EFAULT:
			throw "Error setting struct";
		case EINTR:
			// Note: In "rem" the remaining time is stored
			throw "Interrupted";
		case EINVAL:
			throw "Illegal value";
		}
	}
}



Timezone::Timezone() {
	struct timezone tz;
	struct timeval tv;

	if(gettimeofday(&tv, &tz) < 0) throw "Error getting timezone from system";
	this->tz_minuteswest = tz.tz_minuteswest;
	this->tz_dsttime = tz.tz_dsttime;
}

Timezone::Timezone(int minuteswest) {
	struct timezone tz;
	struct timeval tv;

	if(gettimeofday(&tv, &tz) < 0) throw "Error getting timezone from system";
	this->tz_dsttime = tz.tz_dsttime;
	this->tz_minuteswest = minuteswest;
}

Timezone::Timezone(int minuteswest, int dsttime) {
	this->tz_minuteswest = minuteswest;
	this->tz_dsttime = dsttime;
}

Timezone::~Timezone() {

}

int Timezone::minutesWest(void) const { return this->tz_minuteswest; }
int Timezone::dstTime(void) const { return this->tz_dsttime; }


	
DateTime& DateTime::operator-=(const DateTime &src) {
	this->t -= src.t;
	return *this;
}

DateTime& DateTime::operator+=(const DateTime &src) {
	this->t += src.t;
	return *this;
}

DateTime DateTime::operator+(const DateTime &src) const {
	DateTime ret(this->t);
	ret.t += src.t;
	return ret;
}

DateTime DateTime::operator-(const DateTime &src) const {
	DateTime ret(this->t);
	ret.t -= src.t;
	return ret;
}

void DateTime::setYear(const int year) {
	// XXX: Check year
	
	time_t rawtime = this->t / 1000L;
	const int milliseconds = (int)(this->t % 1000L);
	struct tm info;
	localtime_r(&rawtime, &info);
	info.tm_year = (year-1900);
	rawtime = mktime(&info);
	this->t = rawtime*1000L + milliseconds;
}

void DateTime::setMonth(const int month) {
	if(month < 1 || month > 12) throw "Illegal month";
	
	time_t rawtime = this->t / 1000L;
	const int milliseconds = (int)(this->t % 1000L);
	struct tm info;
	localtime_r(&rawtime, &info);
	info.tm_mon = month-1;
	rawtime = mktime(&info);
	this->t = rawtime*1000L + milliseconds;
}

void DateTime::setDay(const int day) {
	if(day < 1 || day > 31) throw "Illegal month";
	
	time_t rawtime = this->t / 1000L;
	const int milliseconds = (int)(this->t % 1000L);
	struct tm info;
	localtime_r(&rawtime, &info);
	info.tm_mday = day;
	rawtime = mktime(&info);
	this->t = rawtime*1000L + milliseconds;
}

void DateTime::setHour(const int hour) {
	if(hour < 0 || hour > 23) throw "Illegal hour";
	
	time_t rawtime = this->t / 1000L;
	const int milliseconds = (int)(this->t % 1000L);
	struct tm info;
	localtime_r(&rawtime, &info);
	info.tm_hour = hour;
	rawtime = mktime(&info);
	this->t = rawtime*1000L + milliseconds;
}

void DateTime::setMinute(const int minute) {
	if(minute < 0 || minute > 60) throw "Illegal minute";
	
	time_t rawtime = this->t / 1000L;
	const int milliseconds = (int)(this->t % 1000L);
	struct tm info;
	localtime_r(&rawtime, &info);
	info.tm_min = minute;
	rawtime = mktime(&info);
	this->t = rawtime*1000L + milliseconds;
}

void DateTime::setSecond(const int second) {
	if(second < 0 || second > 60) throw "Illegal second";
	
	time_t rawtime = this->t / 1000L;
	const int milliseconds = (int)(this->t % 1000L);
	struct tm info;
	localtime_r(&rawtime, &info);
	info.tm_sec = second;
	rawtime = mktime(&info);
	this->t = rawtime*1000L + milliseconds;
}

void DateTime::setMilliseconds(const int millis) {
	if(millis < 0 || millis > 1000) throw "Illegal milliseconds";
	
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	rawtime = mktime(&info);
	this->t = rawtime*1000L + millis;

}

std::string DateTime::format(const char* format) {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	char buffer[160];
	localtime_r(&rawtime, &info);
	strftime(buffer,160,format, &info);
	return std::string(buffer);
}


int DateTime::year(void) const {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	return info.tm_year + 1900;
}

int DateTime::month(void) const {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	return info.tm_mon + 1;
}

int DateTime::day(void) const {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	return info.tm_mday;
}

int DateTime::dayOfWeek(void) const {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	return info.tm_wday;
}

int DateTime::dayOfYear(void) const {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	return info.tm_yday;
}

int DateTime::hour(void) const {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	return info.tm_hour;
}

int DateTime::minute(void) const {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	return info.tm_min;
}

int DateTime::second(void) const {
	time_t rawtime = this->t / 1000L;
	struct tm info;
	localtime_r(&rawtime, &info);
	return info.tm_sec;
}


}

