
# Default compiler and compiler flags
CXX=g++
CC=gcc

# Default flags for all compilers
O_FLAGS=-Wall -Wextra -O0 -g2
# Debugging flags
#O_FLAGS=-Wall -Werror -Wextra -pedantic -O0 -g2
CXX_FLAGS=$(O_FLAGS)
CC_FLAGS=$(O_FLAGS) --std=c99


# Binaries, object files, libraries and stuff
LIBS=-lsqlite3 -pthread
INCLUDE=
OBJS=TSL2561.o htu21dflib.o database.o
BINS=monitor meteo


# Default generic instructions
default:	all
all:	$(OBJS) $(BINS)
clean:	
	rm -f *.o

TSL2561.o:	TSL2561.c TSL2561.h
	$(CC) $(CC_FLAGS) -o TSL2561.o -c TSL2561.c -D_DEFAULT_SOURCE

htu21dflib.o: htu21dflib.c htu21dflib.h
#	$(CC) $(CC_FLAGS) -o htu21dflib -DHTU21DFTEST htu21dflib.c
	$(CC) $(CC_FLAGS) -c -o htu21dflib.o htu21dflib.c -D_DEFAULT_SOURCE

database.o:	database.cpp database.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<

monitor:	monitor.cpp monitor.hpp udp.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) -o monitor monitor.cpp $(OBJS) $(LIBS)

meteo:	meteo.cpp monitor.hpp
	$(CXX) $(CXX_FLAGS) -o meteo meteo.cpp

