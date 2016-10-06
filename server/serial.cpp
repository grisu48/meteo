/**
  * Serial.cpp
  * Access to the serial port, main purpose to communicate with the Arduino
  * board
  *
  * MIT license, Felix Niederwanger
  * http://opensource.org/licenses/MIT
  */

#include <cerrno>

#include <iostream>
#include <string>
#include <sstream>

#include "serial.hpp"


using namespace std;
namespace io {


#if __cplusplus < 201103L

#define SSTR( x ) dynamic_cast< std::ostringstream & >( \
        ( std::ostringstream() << std::dec << x ) ).str()

#endif


Serial::Serial(const char* device, bool nonBlocking) {
    int flags = O_RDWR;
    if(nonBlocking) flags |= O_NONBLOCK;
    fd = open(device, O_RDWR);
    if(fd < 0) throw "Error opening device";
    bzero(&tio, sizeof(tio));
    setBaudRate(this->_speed);
    this->_eof = false;

    /* Setting port Stuff */
    tio.c_cflag     &=  ~PARENB;            // Make 8n1
    tio.c_cflag     &=  ~CSTOPB;
    tio.c_cflag     &=  ~CSIZE;
    tio.c_cflag     |=  CS8;

    tio.c_cflag     &=  ~CRTSCTS;           // no flow control
    tio.c_cc[VMIN]   =  1;                  // read doesn't block
    tio.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    tio.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&tio);

    /* Flush Port, then applies attributes */
    tcflush( fd, TCIFLUSH );
    if ( tcsetattr ( fd, TCSANOW, &tio ) != 0) {
    	throw "Error from tcsetattr";
    }

    // We are good to go now :-)
}

Serial::Serial(std::string device, bool nonBlocking) : Serial(device.c_str(), nonBlocking) {}

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
    if(ret < 0) throw "Error setting output baud rate";
    ret = cfsetispeed( &tio, this->_speed );
    if(ret < 0) throw "Error setting input baud rate";
    tcsetattr( this->fd, TCSANOW, &tio );
    if(ret < 0) throw "Error applying baud rate";
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
	if(this->isNonBlocking()) throw "Cannot readLine in nonblocking mode";
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
	if(read(&c,1) <= 0) throw "Read failed";
	return (*this);
}

Serial& Serial::operator>>(unsigned char &c) {
	if(read(&c,1) <= 0) throw "Read failed";
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

}
