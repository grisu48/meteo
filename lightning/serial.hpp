/* =============================================================================
 *
 * Title:         Access to the serial console
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2015 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * Description:   General access to the TTY console
 *                The main purpose is to be able to communicate with the
 *                arduino
 * =============================================================================
 */

#ifndef _FLEXLIB_SERIAL_HPP_
#define _FLEXLIB_SERIAL_HPP_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#include <string>
#include <sstream>

using namespace std;

/**
 * Simplified access to the serial console
 */
class Serial {
private:
	struct termios tio;

	/** File descriptor */
	int fd;

	/** Current baud rate */
	speed_t _speed = B9600;

	/** Apply the following flags to the file descriptor */
	void setIOFlags(int flags);
	/** Unset the following flags to the file descriptor */
	void unsetIOFlags(int flags);

	/** eof flag */
	bool _eof;
public:
	Serial(const char* device, bool nonBlocking = false);
	Serial(std::string device, bool nonBlocking = false);
	~Serial();


	/** Set baud rate
	  * Available methods are: B9000 B1920 B38400 B57600 B115200
	  */
	void setBaudRate(speed_t speed);
	/** Set baud rate
	  * Available methods are: B9000 B1920 B38400 B57600 B115200
	  */
	void setSpeed(speed_t speed);

	/** Activate nonblocking mode */
	void setNonBlocking(void);
	/** Activate blocking mode */
	void setBlocking(void);

	/** @returns the current set BAUD rate */
	speed_t speed(void);

	/** @returns true if nonblocking mode is enabled */
	bool isNonBlocking(void);

	/** Read from the serial port
	  * @returns The number of bytes read, zero indicates the end of file
	*/
	virtual ssize_t read(void *buf, size_t count);

	/** Write to the serial port
	  * @returns The number of bytes read, zero indicates the end of file
	*/
	virtual ssize_t write(const void *buf, size_t count);

	/**
	  * Reads the next line and returns it.
	  * If the write fails or an error happens, a  is thrown
	  * If nonblocking mode is enabled, this call throws an 
	  */
	std::string readLine(void);

	/**
	  * When a read reaches the end, the EOF flag is set.
	  * In nonblocking mode this eof check doesn't work guaranteed,
	  * since this is indistinguishable from an error. I try a best-efford
	  * method to provide an eof check
	  *@returns true if EOF has been reached
	  */
	bool eof(void);

	/** Closes the serial port */
	void close(void);

	/** Read a single character
	 * @param where the character should be read into
	 * @returns Reference to this instance
	 * */
	Serial& operator>>(char&);

	/** Read a single character as unsigned char
	 * @param where the character should be read into
	 * @returns Reference to this instance
	 * */
	Serial& operator>>(unsigned char&);

	/**
	 * Writes the given string to the serial port
	 * @returnsReference to this instance
	 */
	Serial& operator<<(std::string&);
	/**
	 * Writes the given character to the serial port
	 * @returnsReference to this instance
	 */
	Serial& operator<<(char&);
	/**
	 * Writes the given character to the serial port
	 * @returnsReference to this instance
	 */
	Serial& operator<<(unsigned char&);
	/**
	 * Writes the given integer to the serial port
	 * @returnsReference to this instance
	 */
	Serial& operator<<(int&);
	/**
	 * Writes the given long value to the serial port
	 * @returnsReference to this instance
	 */
	Serial& operator<<(long&);
	/**
	 * Writes the given float to the serial port
	 * @returnsReference to this instance
	 */
	Serial& operator<<(float&);
	/**
	 * Writes the given double to the serial port
	 * @returnsReference to this instance
	 */
	Serial& operator<<(double&);
};

#if __cplusplus < 201103L

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

#endif


Serial::Serial(const char* device, bool nonBlocking) {
    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = CS8|CREAD|CLOCAL;		// 8n1
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 5;

    int flags = O_RDWR;
    if(nonBlocking) flags |= O_NONBLOCK;
    fd = open(device, O_RDWR);
    if(fd < 0) throw ("Error opening device");
    setBaudRate(this->_speed);
    this->_eof = false;
    // We are good to go now :-)
}

Serial::Serial(std::string device, bool nonBlocking)  {
	memset(&tio, 0, sizeof(tio));
	tio.c_cflag = CS8|CREAD|CLOCAL;		// 8n1
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 5;

	int flags = O_RDWR;
	if(nonBlocking) flags |= O_NONBLOCK;
	fd = open(device.c_str(), O_RDWR);
	if(fd < 0) throw ("Error opening device");
	setBaudRate(this->_speed);
	this->_eof = false;
	// We are good to go now :-)
}

Serial::~Serial() {
	this->close();
}

void Serial::setSpeed(speed_t speed) {
	this->setBaudRate(speed);
}

void Serial::setBaudRate(speed_t speed) {
	if(this->fd <= 0) return;
	int ret;

    ret = cfsetospeed( &tio, this->_speed );
    if(ret < 0) throw ("Error setting output baud rate");
    ret = cfsetispeed( &tio, this->_speed );
    if(ret < 0) throw ("Error setting input baud rate");
    tcsetattr( this->fd, TCSANOW, &tio );
    if(ret < 0) throw ("Error applying baud rate");
	this->_speed = speed;
}

ssize_t Serial::read(void *buf, size_t count) {
	if(this->fd <= 0) return 0;
	ssize_t result = ::read(this->fd, buf, count);
	if(result == 0 && errno != 0)
		_eof = true;
	return result;
}

ssize_t Serial::write(const void *buf, size_t count) {
	if(this->fd <= 0) return 0;
	return ::write(this->fd, buf, count);
}

void Serial::close(void) {
	if(this->fd <= 0) return;
	::close(this->fd);
	this->fd = 0;
}


speed_t Serial::speed(void) { return this->_speed; }
bool Serial::eof(void) { return this->_eof; }

bool Serial::isNonBlocking(void) {
	if(this->fd <= 0) return false;
	int flags = fcntl(fd, F_GETFL, 0);
	return (flags & O_NONBLOCK) != 0;
}


std::string Serial::readLine(void) {
	if(this->fd <= 0) return "";
	if(this->isNonBlocking()) throw ("Cannot readLine in nonblocking mode");
	stringstream ss;

	char c;
	while(true) {
		if(read(&c,1)==0) break;
		if(c == '\n') break;
		else
			ss << c;
	}

	return ss.str();

}

void Serial::setIOFlags(int flags) {
	if(this->fd <= 0) return;
	int fd_flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, fd_flags | flags);
}

void Serial::unsetIOFlags(int flags) {
	if(this->fd <= 0) return;
	int fd_flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, fd_flags & ~flags);
}

void Serial::setNonBlocking(void) {
	setIOFlags(O_NONBLOCK);
}

void Serial::setBlocking(void) {
	unsetIOFlags(O_NONBLOCK);
}

Serial& Serial::operator>>(char &c) {
	if(read(&c,1) <= 0) throw ("Read failed");
	return (*this);
}

Serial& Serial::operator>>(unsigned char &c) {
	if(read(&c,1) <= 0) throw ("Read failed");
	return (*this);
}

Serial& Serial::operator<<(string &str) {
	const char* c = str.c_str();
	write((void*)c, (size_t)str.length());
	return (*this);
}

Serial& Serial::operator<<(unsigned char &c) {
	write(&c,1);
	return (*this);
}

Serial& Serial::operator<<(char &c) {
	write(&c,1);
	return (*this);
}

Serial& Serial::operator<<(int &i) {
#if __cplusplus < 201103L
	string str = SSTR(i);
#else
	string str = to_string(i);
#endif
	return (*this) << str;
}

Serial& Serial::operator<<(long &l) {
#if __cplusplus < 201103L
	string str = SSTR(l);
#else
	string str = to_string(l);
#endif
	return (*this) << str;
}

Serial& Serial::operator<<(float &f) {
#if __cplusplus < 201103L
	string str = SSTR(f);
#else
	string str = to_string(f);
#endif
	return (*this) << str;
}

Serial& Serial::operator<<(double &d) {
#if __cplusplus < 201103L
	string str = SSTR(d);
#else
	string str = to_string(d);
#endif
	return (*this) << str;
}

#endif

