

# Default compiler and compiler flags
CXX=g++
CC=gcc

# Default flags for all compilers
#O_FLAGS=-Wall -Wextra -pedantic -O0 -g2
# Debugging flags
O_FLAGS=-Wall -Werror -Wextra -pedantic -O0 -g2
CXX_FLAGS=$(O_FLAGS) -std=c++11
CC_FLAGS=$(O_FLAGS) -std=c99


# Binaries, object files, libraries and stuff
LIBS=-lm -pthread -lmosquitto
INCLUDE=
OBJS=sensor.o bmp180.o tsl2561.o mcp9808.o htu21df.o config.o string.o
BINS=bmp180 tsl2561 mcp9808 htu21df meteo
LIB_FILE=libmeteo.so

# Default generic instructions
default:	all
all:	$(BINS) 
clean:	
	rm -f *.o
# Object files
%.o:	%.cpp %.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE) $(LIBS)
	
htu21df.o:	htu21df.cpp htu21df.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDE) $(LIBS)

libmeteo.so:	sensors.cpp
	$(CXX) -fPIC $(CXX_FLAGS) -shared -Wl,-soname,$(LIB_FILE) -o $@ $< $(INCLUDE) $(LIBS)

install:
	mkdir -p /usr/include/libmeteo
	install *.hpp /usr/include/libmeteo/
	install $(LIB_FILE) /usr/lib/

example:	example.cpp $(OBJS) 
	$(CXX) $(CXX_FLAGS) -o $@ $< $(INCLUDE) $(LIBS) bmp180.o

bmp180:	read_bmp180.cpp	bmp180.o sensor.o
	$(CXX) $(CXX_FLAGS) -o $@ $< $(INCLUDE) $(LIBS) sensor.o bmp180.o

tsl2561:	read_tsl2561.cpp tsl2561.o sensor.o
	$(CXX) $(CXX_FLAGS) -o $@ $< $(INCLUDE) $(LIBS) sensor.o tsl2561.o

mcp9808:	read_mcp9808.cpp mcp9808.o sensor.o
	$(CXX) $(CXX_FLAGS) -o $@ $< $(INCLUDE) $(LIBS) sensor.o mcp9808.o

htu21df:	read_htu21df.cpp htu21df.o sensor.o
	$(CXX) $(CXX_FLAGS) -o $@ $< $(INCLUDE) $(LIBS) sensor.o htu21df.o

meteo:	meteo.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ $< $(INCLUDE) $(LIBS) $(OBJS)

meteo2:	meteo2.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ $< $(INCLUDE) $(LIBS) -lmosquitto $(OBJS)
