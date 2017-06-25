/* =============================================================================
 *
 * Title:         Time functions. This header file is meant as a standalone
 * Author:        Felix Niederwanger, 2017 <felix@feldspaten.org>
 * License:       GPLv3 (https://www.gnu.org/licenses/gpl-3.0.en.html)
 *
 * =============================================================================
 */

#ifndef _LAZY_TIME_HPP_
#define _LAZY_TIME_HPP_

#include <stdint.h>
#include <sys/time.h>

namespace lazy {

/** Milliseconds since EPOC */
int64_t getSystemTime(void);
/** Nanoseconds since EPOC */
int64_t getSystemTimeMicros(void);

void Sleep(long millis, long micros = 0);


/**
 * Class containing informations about the current timezone
 */
class Timezone {
private:
	int tz_minuteswest;
	int tz_dsttime;
public:
	/** Create new timezone with the values from the system */
	Timezone();
	/** Creates new timezone with the given minutes west of Greenwich and the DST time from the system */
	Timezone(int minuteswest);
	/** Creates new timezone
	 * @param minuteswest minutes west of Greenwich
	 * @param dsttime Daylight saving time correction type (according to the system)
	 * */
	Timezone(int minuteswest, int dsttime);
	virtual ~Timezone();

	/**
	 * Get the minutes west
	 */
	int minutesWest(void) const;

	/**
	 * Daylight saving time. Available return values are
	 *     DST_NONE      on DST
           DST_USA       USA style DST
           DST_AUST      Australian style DST
           DST_WET       Western European DST
           DST_MET       Middle European DST
           DST_EET       Eastern European DST
           DST_CAN       Canada
           DST_GB        Great Britain and Eire
           DST_RUM       Romania
           DST_TUR       Turkey
           DST_AUSTALT   Australian style with shift in 1986

      This values have never been used under Linux and are purely of historic interest.
	 */
	int dstTime(void) const;

};

/**
  * Datetime manipulation class
  */
class DateTime {
private:
	/** Timestamp in milliseconds since EPOC */
	long t;
	
	
public:
	/**
	  * New DateTime instance with the current timestamp
	  */
	DateTime() {
		this->t = getSystemTime();
	}
	/** Instanciate a new DateTime instance using the given timestamp
	  * @param timestamp Milliseconds since EPOC
	  */
	DateTime(const long timestamp) {
		this->t = timestamp;
	}
	/** Copy the given DateTime instance
	  * @param src Source of the copy
	  */
	DateTime(const DateTime &src) {
		this->t = src.t;
	}
	virtual ~DateTime() {}
	
	/** Sets the year of the instance  */
	void setYear(const int year);
	/** Sets the month of the instance. 1 = January, 12 = December */
	void setMonth(const int month);
	/** Sets the day of month of the instance  */
	void setDay(const int day);
	/** Sets the hour of month of the instance  */
	void setHour(const int hour);
	/** Sets the minute of month of the instance  */
	void setMinute(const int minute);
	/** Sets the second of month of the instance  */
	void setSecond(const int second);
	/** Sets the milliseconds of the instance  */
	void setMilliseconds(const int millis);
	
	/** Set timestamp to now */
	void setNow() { this->t = getSystemTime(); }
	
	/** Get the milliseconds since epoc */
	long timestamp() const { return this->t; }
	/** Get the milliseconds since epoc */
	long getMilliSecondsSinceEpoc() const { return this->t; }
	
	int year(void) const;
	int month(void) const;
	/** @returns day of the month */
	int day(void) const;
	/** @return day of the week */
	int dayOfWeek(void) const;
	/** @return day of the year */
	int dayOfYear(void) const;
	int hour(void) const;
	int minute(void) const;
	int second(void) const;
	
	/** Formats the given DateTime instance according to the given format.
	  * Available options are:
	  * <ul>
	  * <li>%H - Hour (24h), %$I - hour (12h)</li>
	  * <li>%M - Minute</li>
	  * <li>%S - Seconds</li>
	  * <li>%T - Time in 24h notion (%H:%M:%S)</li>
	  * <li>%Y - Year including century, %y year without century</li>
	  * <li>%m - Month</li>
	  * <li>%d - Day of month</li>
	  * <li>%F - USO 8601 format (%Y-%m-%d)</li>
	  * </ul>
	  * See the function strftime for details.
	  * @param format string prototype
	  */
	std::string format(const char* format);
	
	/** Formats the given DateTime instance according to the given format.
	  * Available options are:
	  * <ul>
	  * <li>%H - Hour (24h), %$I - hour (12h)</li>
	  * <li>%M - Minute</li>
	  * <li>%S - Seconds</li>
	  * <li>%T - Time in 24h notion (%H:%M:%S)</li>
	  * <li>%Y - Year including century, %y year without century</li>
	  * <li>%m - Month</li>
	  * <li>%d - Day of month</li>
	  * <li>%F - USO 8601 format (%Y-%m-%d)</li>
	  * </ul>
	  * See the function strftime for details.
	  * @param format string prototype
	  */
	std::string format(const std::string &format) { return this->format(format.c_str()); }
	
	std::string format() { return this->format("%Y-%m-%d %H:%M:%S"); }
	
	DateTime& operator-=(const DateTime &src);
	DateTime& operator+=(const DateTime &src);
	DateTime operator+(const DateTime &src) const;
	DateTime operator-(const DateTime &src) const;
	bool operator==(const DateTime &src) const { return this->t == src.t; }
	bool operator==(const long timestamp) const { return (this->t == timestamp); }
	bool operator!=(const DateTime &src) const { return this->t != src.t; }
	bool operator!=(const long timestamp) const { return this->t != timestamp; }
	bool operator<(const DateTime &src) const { return this->t < src.t; }
	bool operator<=(const DateTime &src) const { return this->t <= src.t; }
	bool operator>(const DateTime &src) const { return this->t > src.t; }
	bool operator>=(const DateTime &src) const { return this->t >= src.t; }
};



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

#endif
