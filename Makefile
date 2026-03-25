.PHONY: all clean build play_mas perft_test run run-play-mas run-perft debug release run-debug run-release run-perft-debug run-perft-release help

# Variables
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Isrc
LDFLAGS =
DEPFLAGS = -MMD -MP
RELEASE_FLAGS = -O3 -flto
BASE_BUILD_DIR = build
MODE ?= default
SRCS = $(wildcard src/*.cpp)

DEBUG_FLAGS = -O0 -g -DDEBUG

ifeq ($(MODE),default)
MODE_CXXFLAGS =
OUT_DIR = $(BASE_BUILD_DIR)
else ifeq ($(MODE),debug)
MODE_CXXFLAGS = $(DEBUG_FLAGS)
OUT_DIR = $(BASE_BUILD_DIR)/debug
else ifeq ($(MODE),release)
MODE_CXXFLAGS = $(RELEASE_FLAGS)
OUT_DIR = $(BASE_BUILD_DIR)/release
else
$(error Invalid MODE '$(MODE)'. Use MODE=default, MODE=debug, or MODE=release)
endif

EFFECTIVE_CXXFLAGS = $(CXXFLAGS) $(MODE_CXXFLAGS)
EFFECTIVE_LDFLAGS = $(LDFLAGS)

# Sources with their own main() that should not be part of shared objects
OTHER_MAIN_SRCS =
PLAY_MAS_SRC = src/play_mas.cpp
PERFT_TEST_SRC = src/perft_test.cpp

LIB_SRCS = $(filter-out $(PLAY_MAS_SRC) $(PERFT_TEST_SRC) $(OTHER_MAIN_SRCS),$(SRCS))

LIB_OBJS = $(patsubst src/%.cpp,$(OUT_DIR)/%.o,$(LIB_SRCS))
PLAY_MAS_OBJ = $(patsubst src/%.cpp,$(OUT_DIR)/%.o,$(PLAY_MAS_SRC))
PERFT_TEST_OBJ = $(patsubst src/%.cpp,$(OUT_DIR)/%.o,$(PERFT_TEST_SRC))

PLAY_MAS_TARGET = $(OUT_DIR)/play_mas
PERFT_TEST_TARGET = $(OUT_DIR)/perft_test
BUILD_STAMP = $(OUT_DIR)/.dir-stamp

DEPS = $(LIB_OBJS:.o=.d) $(PLAY_MAS_OBJ:.o=.d) $(PERFT_TEST_OBJ:.o=.d)

# Default target
all: build

# Build target
build: play_mas perft_test

play_mas: $(PLAY_MAS_TARGET)

perft_test: $(PERFT_TEST_TARGET)

$(PLAY_MAS_TARGET): $(LIB_OBJS) $(PLAY_MAS_OBJ) | $(BUILD_STAMP)
	$(CXX) $(EFFECTIVE_CXXFLAGS) -o $@ $^ $(EFFECTIVE_LDFLAGS)

$(PERFT_TEST_TARGET): $(LIB_OBJS) $(PERFT_TEST_OBJ) | $(BUILD_STAMP)
	$(CXX) $(EFFECTIVE_CXXFLAGS) -o $@ $^ $(EFFECTIVE_LDFLAGS)

$(OUT_DIR)/%.o: src/%.cpp | $(BUILD_STAMP)
	$(CXX) $(EFFECTIVE_CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_STAMP):
	mkdir -p $(OUT_DIR)
	touch $(BUILD_STAMP)

# Run target
run: run-play-mas

run-play-mas: play_mas
	./$(PLAY_MAS_TARGET)

run-perft: perft_test
	./$(PERFT_TEST_TARGET)

# Debug / release wrapper targets
debug:
	$(MAKE) MODE=debug build

release:
	$(MAKE) MODE=release build

run-debug:
	$(MAKE) MODE=debug run

run-release:
	$(MAKE) MODE=release run

run-perft-debug:
	$(MAKE) MODE=debug run-perft

run-perft-release:
	$(MAKE) MODE=release run-perft

# Clean target
clean:
	rm -rf $(BASE_BUILD_DIR)

# Help target
help:
	@echo "Available targets:"
	@echo "  make build        - Compile play_mas and perft_test"
	@echo "  make play_mas     - Compile play_mas executable"
	@echo "  make perft_test   - Compile perft_test executable"
	@echo "  make run          - Build and run play_mas"
	@echo "  make run-play-mas - Build and run play_mas"
	@echo "  make run-perft    - Build and run perft_test"
	@echo "  make debug        - Build both targets in debug mode"
	@echo "  make release      - Build both targets in release mode"
	@echo "  make run-debug    - Build/run play_mas in debug mode"
	@echo "  make run-release  - Build/run play_mas in release mode"
	@echo "  make run-perft-debug   - Build/run perft_test in debug mode"
	@echo "  make run-perft-release - Build/run perft_test in release mode"
	@echo "  make MODE=debug build  - Build with explicit mode"
	@echo "  make MODE=release run  - Run play_mas in release mode"
	@echo "  make clean        - Remove compiled files"
	@echo "  make help         - Show this message"

-include $(DEPS)