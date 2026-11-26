CC=gcc
CXX=g++
CFLAGS=-std=c11 -Wall -Wextra -O2
CXXFLAGS=-std=c++17 -Wall -Wextra -O2 -pthread
monitor: main.cpp metrics.o metrics.h
	$(CXX) $(CXXFLAGS) main.cpp metrics.o -o monitor
metrics.o: metrics.c metrics.h
	$(CC) $(CFLAGS) -c metrics.c -o metrics.o
.PHONY: clean
clean:
	rm -f monitor metrics.o
