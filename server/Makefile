O_FLAGS=-Og -g2 -Wall -Werror -pedantic -Wextra

CXX=g++
CXX_FLAGS=$(O_FLAGS) -std=c++11

INCLUDE=
LIBS=-pthread -lsqlite3 -lmosquitto


OBJS=string.o tcp.o udp.o node.o server.o parser.o serial.o database.o config.o mosquitto.o


default:	all
all:	main
clean:
	rm -rf *.o


main:	main.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) -o meteod $< $(OBJS) $(INCLUDE) $(LIBS) 

meteo_v2:	main_v2.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) -o meteod $< $(OBJS) $(INCLUDE) $(LIBS) 

%.o:	%.cpp %.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<
	
#database.o:	database.cpp database.hpp
#	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE) `mysql_config --libs`

