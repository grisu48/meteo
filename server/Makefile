O_FLAGS=-Og -g2 -Wall -Werror -pedantic -Wextra

CXX=g++
CXX_FLAGS=$(O_FLAGS) -std=c++11

INCLUDE=
LIBS=-pthread -lsqlite3 -lmosquitto


OBJS=config.o mosquitto.o string.o time.o collector.o


default:	all
all:	meteod
clean:
	rm -rf *.o


meteod:	meteod.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) -o meteod $< $(OBJS) $(INCLUDE) $(LIBS) 

%.o:	%.cpp %.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<
	
#database.o:	database.cpp database.hpp
#	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE) `mysql_config --libs`

