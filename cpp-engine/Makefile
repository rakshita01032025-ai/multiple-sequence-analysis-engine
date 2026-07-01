# =============================================================================
# Multi-Sequence Analysis Engine — C++ Engine Makefile
# =============================================================================
#
# Targets
# -------
#   all       (default) Build the production engine binary: mse_engine
#   test      Build and run the test binary:                mse_engine_tests
#   clean     Remove all build artefacts
#
# Usage
# -----
#   make              # build mse_engine
#   make test         # build and run mse_engine_tests
#   make clean        # remove bin/
#
# Requirements
# ------------
#   g++ with C++17 support (GCC 7+ or Clang 5+)
#   Linux (for /proc/self/status memory reporting; see getPeakMemoryKb)
#
# =============================================================================

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic -I.

# Source files shared by both the engine binary and the test binary.
SHARED_SRC := algorithms.cpp

# Engine binary
ENGINE_MAIN := main.cpp
ENGINE_BIN  := bin/mse_engine

# Test binary
TEST_MAIN   := test_algorithms.cpp
TEST_BIN    := bin/mse_engine_tests

# Default target
.PHONY: all
all: $(ENGINE_BIN)

# Create output directory
bin:
	mkdir -p bin

# Production engine binary
$(ENGINE_BIN): $(SHARED_SRC) $(ENGINE_MAIN) | bin
	$(CXX) $(CXXFLAGS) $(SHARED_SRC) $(ENGINE_MAIN) -o $(ENGINE_BIN)
	@echo "Built: $(ENGINE_BIN)"

# Test binary (built and run by 'make test')
$(TEST_BIN): $(SHARED_SRC) $(TEST_MAIN) | bin
	$(CXX) $(CXXFLAGS) $(SHARED_SRC) $(TEST_MAIN) -o $(TEST_BIN)
	@echo "Built: $(TEST_BIN)"

.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN)

.PHONY: clean
clean:
	rm -rf bin