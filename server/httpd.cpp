

#include <string>
#include <sstream>

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>


/** Socket helper */
class Socket {
private:
	int sock;
	
	ssize_t write(const void* buf, size_t len) {
		if(sock <= 0) throw "Socket closed";
		ssize_t ret = ::send(sock, buf, len, 0);
		if(ret == 0) {
			this->close();
			throw "Socket closed";
		} else if (ret < 0) 
			throw strerror(errno);
		else
			return ret;
	}
	
public:
	Socket(int sock) { this->sock = sock; }
	virtual ~Socket() { this->close(); }
	void close() {
		if(sock > 0) {
			::close(sock);
			sock = 0;
		}
	}
	
	std::string getRemoteAddress() {
		struct sockaddr address;
		socklen_t address_len;
		
		if( getpeername(this->sock, &address, &address_len) < 0) return "";
		struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&address;
		struct in_addr ipAddr = pV4Addr->sin_addr;
		char str[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN );
		return std::string(str);
	}
	
	void print(const char* str) {
	    const size_t len = strlen(str);
		if(len == 0) return;
		this->write((void*)str, sizeof(char)*len);
	}
	
	void println(const char* str) {
	    this->print(str);
		this->write("\n", sizeof(char));
	}
	
	void print(const std::string &str) {
	    const size_t len = str.size();
		if(len == 0) return;
		this->write((void*)str.c_str(), sizeof(char)*len);
	}
	
	void println(const std::string &str) {
	    this->print(str);
		this->write("\n", sizeof(char));
	}
	
	Socket& operator<<(const char c) {
		this->write(&c, sizeof(char));
		return (*this);
	}
	Socket& operator<<(const char* str) {
		print(str);
		return (*this);
	}
	Socket& operator<<(const std::string &str) {
		this->print(str);
		return (*this);
	}
	
	template<typename T>
	Socket& operator<<(const T &t) {
	    std::string str = std::to_string(t);
	    this->print(str);
	    return (*this);
	}
	
	Socket& operator>>(char &c) {
		if(sock <= 0) throw "Socket closed";
		ssize_t ret = ::recv(sock, &c, sizeof(char), 0);
		if(ret == 0) {
			this->close();
			throw "Socket closed";
		} else if (ret < 0) 
			throw strerror(errno);
		else
			return (*this);
	}
	
	Socket& operator>>(std::string &str) {
		std::string line = readLine();
		str = line;
		return (*this);
	}
	
	std::string readLine(void) {
		if(sock <= 0) throw "Socket closed";
		std::stringstream ss;
		while(true) {
			char c;
			ssize_t ret = ::recv(sock, &c, sizeof(char), 0);
			if(ret == 0) {
				this->close();
				throw "Socket closed";
			} else if (ret < 0) 
				throw strerror(errno);
				
			if(c == '\n')
				break;
			else
				ss << c;
		}
		std::string ret = ss.str();
		return ret;
	}
	
	void writeHttpHeader(const int statusCode = 200, std::string contentType = "text/html") {
            this->print("HTTP/1.1 ");
            this->print(std::to_string(statusCode));
            this->print(" OK\n");
            this->print("Content-Type:");
            this->print(contentType);
            this->print("\n\n");
	}
};

