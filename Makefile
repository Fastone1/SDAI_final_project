.PHONY: all clean build run release debug help

# Variables
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20 -Isrc
LDFLAGS =
DEBUG_FLAGS = -g
RELEASE_FLAGS = -O3 -march=native -flto
TARGET = program
BUILD_DIR = build
SRCS = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Default target
all: build

# Build target
build: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Release target
release:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/release CXXFLAGS="$(CXXFLAGS) $(RELEASE_FLAGS)" build

# Debug target
debug:
	$(MAKE) BUILD_DIR=$(BUILD_DIR)/debug CXXFLAGS="$(CXXFLAGS) $(DEBUG_FLAGS)" build

# Run target
run: build
	./$(TARGET)

# Clean target
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  make build   - Compile the project"
	@echo "  make run     - Build and run the program"
	@echo "  make release - Build the program with optimizations"
	@echo "  make debug   - Build the program with debug symbols"
	@echo "  make clean   - Remove compiled files"
	@echo "  make help    - Show this message"