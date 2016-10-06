O_FLAGS=-Og -g2 -Wall -Werror -pedantic -Wextra

CXX=g++
CXX_FLAGS=$(O_FLAGS) -std=c++11

INCLUDE=-I/usr/include/mysql
LIBS=-lmysqlclient -pthread


OBJS=string.o tcp.o udp.o node.o server.o parser.o serial.o database.o


default:	all
all:	main
clean:
	rm -rf *.o


main:	main.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) -o server $< $(OBJS) $(INCLUDE) $(LIBS) 

%.o:	%.cpp %.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<
	
database.o:	database.cpp database.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< -I/usr/include/mysql
