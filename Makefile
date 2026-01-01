# Simple Makefile for DOD Agent System

CXX = g++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic -I./include
DEBUGFLAGS = -std=c++17 -g -O0 -Wall -Wextra -Wpedantic -I./include

TARGET = dod_simulation
SOURCES = src/main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all clean debug run

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)
	@echo "Build complete: $(TARGET)"

debug: $(SOURCES)
	$(CXX) $(DEBUGFLAGS) -o $(TARGET)_debug $(SOURCES)
	@echo "Debug build complete: $(TARGET)_debug"

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET)_debug $(OBJECTS) simulation_log.bin
	@echo "Clean complete"
