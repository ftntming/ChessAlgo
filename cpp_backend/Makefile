TARGET = chess # Can run directly as ./chess

CXX = g++
SEARCH_DEBUG_LOGS ?= 0
SEARCH_PERF_LOGS ?= 1
SEARCH_PARALLEL ?= 1
SEARCH_LOG_DEFINES = -DSEARCH_DEBUG_LOGS=$(SEARCH_DEBUG_LOGS) -DSEARCH_PERF_LOGS=$(SEARCH_PERF_LOGS) -DSEARCH_PARALLEL=$(SEARCH_PARALLEL)
CXXFLAGS = -std=c++17 -Wall -g -pthread -Isrc $(SEARCH_LOG_DEFINES)

SRC = $(wildcard src/*.cpp) $(wildcard src/pieces/*.cpp)
SRC := $(filter-out src/BackendBridge.cpp src/main.cpp, $(SRC))

MAIN = src/main.cpp

OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(MAIN) $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(MAIN) $(SRC)

clean:
	rm -f $(TARGET) $(OBJ)