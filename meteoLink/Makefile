O_FLAGS=-Og -g2 -Wall -Werror -pedantic -Wextra

CXX=g++
CXX_FLAGS=$(O_FLAGS) -std=c++11

INCLUDE=-I/usr/include/mysql
LIBS=-lmysqlclient 


OBJS=serial.o string.o


default:	all
all:	main $(OBJS)
clean:
	rm -rf *.o


main:	main.cpp $(OBJS)
	$(CXX) $(CXX_FLAGS) -o meteo $< $(OBJS) $(INCLUDE) $(LIBS) 

%.o:	%.cpp %.hpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $<
