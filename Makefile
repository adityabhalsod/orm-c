# ORM-C Makefile
# Build system for the ORM library and tests

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11 -I.
LDFLAGS = -lsqlite3 -lpthread

# Directories
SRC_DIR = src
TEST_DIR = tests
EXAMPLE_DIR = examples
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
CORE_SOURCES = $(wildcard $(SRC_DIR)/core/*.c)
UTILS_SOURCES = $(wildcard $(SRC_DIR)/utils/*.c)
BACKEND_SOURCES = $(wildcard $(SRC_DIR)/backends/*/*.c)
ALL_SOURCES = $(CORE_SOURCES) $(UTILS_SOURCES) $(BACKEND_SOURCES)
CORE_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(ALL_SOURCES))

# Test files
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_BINARIES = $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SOURCES))

# Example files
EXAMPLE_SOURCES = $(wildcard $(EXAMPLE_DIR)/*.c)
EXAMPLE_BINARIES = $(patsubst $(EXAMPLE_DIR)/%.c,$(BIN_DIR)/%,$(EXAMPLE_SOURCES))

# Targets
.PHONY: all clean test dirs help examples

all: dirs $(CORE_OBJECTS)
	@echo "Build complete!"

# Create necessary directories
dirs:
	@mkdir -p $(OBJ_DIR)/core
	@mkdir -p $(OBJ_DIR)/utils
	@mkdir -p $(OBJ_DIR)/backends/sqlite
	@mkdir -p $(BIN_DIR)

# Compile core source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Build test executables
$(BIN_DIR)/test_%: $(TEST_DIR)/test_%.c $(CORE_OBJECTS)
	@echo "Building test $@..."
	@$(CC) $(CFLAGS) $< $(CORE_OBJECTS) -o $@ $(LDFLAGS)

# Build example executables
$(BIN_DIR)/%: $(EXAMPLE_DIR)/%.c $(CORE_OBJECTS)
	@echo "Building example $@..."
	@$(CC) $(CFLAGS) $< $(CORE_OBJECTS) -o $@ $(LDFLAGS)

# Build all tests
tests: dirs $(TEST_BINARIES)
	@echo "All tests built!"

# Build all examples
examples: dirs $(EXAMPLE_BINARIES)
	@echo "All examples built!"

# Run all tests
test: tests
	@echo ""
	@echo "==============================================="
	@echo "Running all tests..."
	@echo "==============================================="
	@for test in $(TEST_BINARIES); do \
		echo ""; \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done
	@echo ""
	@echo "==============================================="
	@echo "All tests completed successfully!"
	@echo "==============================================="

# Run specific test
test_%: $(BIN_DIR)/test_%
	@echo "Running $<..."
	@./$<

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete!"

# Help target
help:
	@echo "ORM-C Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build core library objects (default)"
	@echo "  tests        - Build all test executables"
	@echo "  test         - Build and run all tests"
	@echo "  test_<name>  - Build and run a specific test (e.g., test_model)"
	@echo "  examples     - Build all example programs"
	@echo "  clean        - Remove all build artifacts"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make test              # Run all tests"
	@echo "  make test_model        # Run model tests only"
	@echo "  make examples          # Build example programs"
	@echo "  make clean && make test # Clean build and test"

# Dependencies
$(CORE_OBJECTS): $(SRC_DIR)/core/model.h $(SRC_DIR)/core/field_types.h
$(TEST_BINARIES): $(SRC_DIR)/core/model.h $(SRC_DIR)/core/field_types.h
