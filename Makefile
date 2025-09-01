# Makefile for TPX3 Histogram C++ Program
# Prerequisites: nlohmann/json (header-only library)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -g -I/usr/include/nlohmann
LDFLAGS = -pthread

# Target executable
TARGET = tpx3_histogram

# Source files
SOURCES = tpx3_histogram.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"
	@rm -f $(OBJECTS)
	@echo "Object files removed"

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Clean complete"

# Install dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y nlohmann-json3-dev build-essential

# Install dependencies (CentOS/RHEL/Fedora)
install-deps-yum:
	@echo "Installing dependencies..."
	sudo yum install -y nlohmann-json-devel gcc-c++ make

# Create data directory
setup:
	mkdir -p data
	@echo "Data directory created"

# Run the program
run: $(TARGET) setup
	./$(TARGET)

# Run with custom host/port
run-custom: $(TARGET) setup
	@echo "Usage: make run-custom HOST=<host> PORT=<port>"
	@echo "Example: make run-custom HOST=192.168.1.100 PORT=9000"
	@if [ -n "$(HOST)" ] && [ -n "$(PORT)" ]; then \
		./$(TARGET) --host $(HOST) --port $(PORT); \
	else \
		echo "Please specify HOST and PORT variables"; \
		exit 1; \
	fi

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build the program (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  install-deps - Install dependencies (Ubuntu/Debian)"
	@echo "  install-deps-yum - Install dependencies (CentOS/RHEL/Fedora)"
	@echo "  setup        - Create data directory"
	@echo "  run          - Build and run the program"
	@echo "  run-custom   - Run with custom host/port (HOST=<host> PORT=<port>)"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make run"
	@echo "  make run-custom HOST=192.168.1.100 PORT=9000"

# Phony targets
.PHONY: all clean install-deps install-deps-yum setup run run-custom help
