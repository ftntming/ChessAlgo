CXX = g++

CXXFLAGS = -std=c++17 -fPIC \
  -I"$(JAVA_HOME)/include" \
  -I"$(JAVA_HOME)/include/darwin" \
  -Icpp_backend/src

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
	$(MAKE) -C cpp_backend/test

clean:
	rm -f $(TARGET)
	@echo "🧹 Cleaned: $(TARGET)"
    # go into $(JAVA_SRC) and do "mvn clean"
	@echo "🧹 Cleaned: Java bin folder"
