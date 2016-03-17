#!/bin/bash

#gcc -Wall -O2 -o TSL2561.o -c TSL2561.c
#gcc -Wall -O2 -o TSL2561_test.o -c TSL2561_test.c
#gcc -Wall -O2 -o TSL2561_test TSL2561.o TSL2561_test.o


gcc -O2 -Wall -o htu21dflib -DHTU21DFTEST htu21dflib.c
gcc -O2 -Wall -c htu21dflib.c
gcc -Wall -O2 -o TSL2561.o -c TSL2561.c
g++ -O2 -Wall htu21dflib.o TSL2561.o -o monitor monitor.cpp

