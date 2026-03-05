CXX = g++
SEARCH_DEBUG_LOGS ?= 0
SEARCH_PERF_LOGS ?= 1
SEARCH_LOG_DEFINES = -DSEARCH_DEBUG_LOGS=$(SEARCH_DEBUG_LOGS) -DSEARCH_PERF_LOGS=$(SEARCH_PERF_LOGS)
CXXFLAGS = -std=c++17 -Wall -g -Isrc \
  -I../src \
  -I"$(JAVA_HOME)/include" \
  -I"$(JAVA_HOME)/include/darwin" \
  $(SEARCH_LOG_DEFINES)

TEST_SRC = $(wildcard test_*.cpp)
TEST_EXES = $(TEST_SRC:.cpp=)

ENGINE_SRC = $(filter-out ../src/main.cpp, $(wildcard ../src/*.cpp) ../src/pieces/*.cpp)

.PHONY: all clean run run_all help

# Build all individual test executables
all: $(TEST_EXES)
	@echo "Built all test executables: $(TEST_EXES)"

# Individual test executables - each test_*.cpp as its own binary
test_%: test_%.cpp $(ENGINE_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "Built: $@"

# Run all individual tests sequentially
run_all: $(TEST_EXES)
	@echo "Running all individual tests..."
	@for test in $(TEST_EXES); do \
		echo ""; \
		echo "===== Running $$test ====="; \
		./$$test || echo "FAILED: $$test"; \
	done

# Run a specific test by name (e.g., make run_test_main)
run_test_%: test_%
	@echo "Running test_$*..."
	@./test_$*

# Special target: build and run test_run_fen (FEN game simulation)
test_run_fen: test_run_fen.cpp $(ENGINE_SRC)
	@echo "Building test_run_fen..."
	$(CXX) $(CXXFLAGS) $^ -o test_run_fen
	@echo "Running test_run_fen..."
	@./test_run_fen

# List all available tests and targets
help:
	@echo "Available test targets:"
	@echo "  make all              - Build all individual test executables"
	@echo "  make run_all          - Run all individual tests sequentially"
	@echo "  make run_test_TESTNAME - Run a specific test (e.g., make run_test_main)"
	@echo ""
	@echo "Available individual tests:"
	@for test in $(TEST_EXES); do \
		echo "  - $$test (run with: make run_test_$$(echo $$test | sed 's/test_//'))"; \
	done

clean:
	rm -f $(TEST_EXES)
	@echo "Cleaned test executables: $(TEST_EXES)"
