CXX := g++
CXXFLAGS := -std=c++23

LINK.o := $(CXX)

.PHONY: all clean handin

main:

main.cpp:

run: ./main

all : main run

clean :
	rm -f main.o 