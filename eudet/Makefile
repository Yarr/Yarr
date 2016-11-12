# Simple Makefile for multiple executeables + X other files/classes
# Main files .cxx, Classes in .cpp and .h
# Define INCLUDE, LIBS and EXE accordingly

MAKEFLAGS=--warn-undefined-variables

INCLUDE = -I./ -I../src/libYarr/include -I../src/libSpec/include -I../src/libUtil/include -I../src/libFei4/include -I../src/libFe65p2/include
LIBS = -L../src/lib -lyarr

# Define compiler and flags
CXX = g++
CFLAGS = -g -O1 -Wall -std=c++11 -pthread $(INCLUDE)
LDFLAGS = $(LIBS)

SRC = $(wildcard eudaq/*.cc) 
TARGET = $(wildcard *.cpp)
OBJ = $(patsubst eudaq/%.cc,build/%.o,$(SRC))
OOBJ = $(patsubst %.cpp,build/%.o,$(TARGET))
EXE = $(patsubst %.cpp,bin/%,$(TARGET))

all: $(OBJ) $(OOBJ) $(EXE)

build/%.o: %.cpp
	@echo "[Compiling] $@"
	@$(CXX) -c $(CFLAGS) $< -o $@

build/%.o: eudaq/%.cc
	@echo "[Compiling] $@"
	@$(CXX) -c $(CFLAGS) $< -o $@

bin/%: build/%.o $(OBJ)
	@echo "[Linking] $@"
	@$(CXX) $(CFLAGS) $(LDFLAGS) $< $(OBJ) $(LIBS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJ) $(OOBJ)
	rm -f $(EXE)
