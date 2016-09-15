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

namespace io {

/**
 * Simplified access to the serial console
 */
class Serial {
private:
	struct termios tio;

	/** File descriptor */
	int fd;

	/** Current baud rate */
	speed_t _speed = B57600;

	/** Apply the following flags to the file descriptor */
	void setIOFlags(int flags);
	/** Unset the following flags to the file descriptor */
	void unsetIOFlags(int flags);

	/** eof flag */
	bool _eof;
public:
	Serial(const char* device, bool nonBlocking = false);
	Serial(std::string device, bool nonBlocking = false);
	virtual ~Serial();


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
	  * If the write fails or an error happens, a IOException is thrown
	  * If nonblocking mode is enabled, this call throws an IOException
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

}

#endif

