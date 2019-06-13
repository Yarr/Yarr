# Simple Makefile for multiple executeables + X other files/classes
# Main files .cxx, Classes in .cpp and .h
# Define INCLUDE, LIBS and EXE accordingly

MAKEFLAGS=--warn-undefined-variables

INCLUDE = -I./. $(shell root-config --cflags) -I../libFei4/include -I../libUtil/include -I../libYarr/include -I../libSpec/include -I../libFe65p2/include
LIBS = $(shell root-config --libs) -L./../lib -lyarr

# Define compiler and flags
CXX = g++
CFLAGS = -g -O0 -Wall $(INCLUDE)
LDFLAGS = $(LIBS)

SRC = $(wildcard *.cpp) 
TARGET = $(wildcard *.cxx)
OBJ = $(SRC:.cpp=.o)
OOBJ = $(TARGET:.cxx=.o)
EXE = $(TARGET:.cxx=)

all: $(EXE) $(OBJ) $(OOBJ)

%.o: %.cxx $(OBJ)
	@$(CXX) -c $(CFLAGS) $< -o $@
	@echo "[Compiling] $@"

%.o: %.cpp 
	@$(CXX) -c $(CFLAGS) $< -o $@
	@echo "[Compiling] $@"

%: %.o
	@$(CXX) $(CFLAGS) $< $(OBJ) $(LDFLAGS) -o $@
	@echo "[Linking] $@"

.PHONY: clean
clean:
	rm -f $(OBJ) $(OOBJ)
	rm -f $(EXE)
