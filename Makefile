CXX = g++

SEARCH_DEBUG_LOGS ?= 0
SEARCH_PERF_LOGS ?= 1
SEARCH_PARALLEL ?= 1
SEARCH_LOG_DEFINES = -DSEARCH_DEBUG_LOGS=$(SEARCH_DEBUG_LOGS) -DSEARCH_PERF_LOGS=$(SEARCH_PERF_LOGS) -DSEARCH_PARALLEL=$(SEARCH_PARALLEL)

CXXFLAGS = -std=c++17 -fPIC -pthread \
  -I"$(JAVA_HOME)/include" \
  -I"$(JAVA_HOME)/include/darwin" \
  -Icpp_backend/src \
  $(SEARCH_LOG_DEFINES)

SRC = cpp_backend/src/BackendBridge.cpp \
      cpp_backend/src/board.cpp \
      cpp_backend/src/movegen.cpp \
      cpp_backend/src/search.cpp \
      cpp_backend/src/evaluate.cpp \
      $(wildcard cpp_backend/src/pieces/*.cpp)

TARGET = libChessEngine.dylib

JAVA_SRC = java_swing_frontend

.PHONY: all java test clean

all: $(TARGET) java test

$(TARGET):
	@echo "🔨 Building $(TARGET)..."
	$(CXX) $(CXXFLAGS) -shared -o $(TARGET) $(SRC)
	@echo "Build complete: $(TARGET)"

java:
	@echo "🔨 Compiling Java sources..."
	# go into $(JAVA_SRC) and do "mvn -Dexec.mainClass=ui.ChessApp exec:java"
	cd $(JAVA_SRC) && CHESS_ENGINE_LIB_PATH=`pwd`/../$(TARGET) mvn -Dexec.mainClass=ui.ChessApp exec:java
	@echo "Java build complete."

test:
	$(MAKE) -C cpp_backend/test SEARCH_DEBUG_LOGS=$(SEARCH_DEBUG_LOGS) SEARCH_PERF_LOGS=$(SEARCH_PERF_LOGS) SEARCH_PARALLEL=$(SEARCH_PARALLEL)

clean:
	rm -f $(TARGET)
	@echo "🧹 Cleaned: $(TARGET)"
    # go into $(JAVA_SRC) and do "mvn clean"
	@echo "🧹 Cleaned: Java bin folder"
