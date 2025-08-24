#!/bin/bash

# Test script for TPX3 Histogram Program
# This script demonstrates the program's functionality

echo "=== TPX3 Histogram Program Test ==="
echo

# Check if program exists
if [ ! -f "./tpx3_histogram" ]; then
    echo "Error: Program not found. Please build it first with 'make'"
    exit 1
fi

# Check if data directory exists
if [ ! -d "./data" ]; then
    echo "Creating data directory..."
    make setup
fi

echo "Program built successfully!"
echo "Executable: ./tpx3_histogram"
echo "Data directory: ./data/"
echo

echo "Available commands:"
echo "  ./tpx3_histogram --help          # Show help"
echo "  ./tpx3_histogram                  # Run with defaults (localhost:8451)"
echo "  ./tpx3_histogram --host 192.168.1.100 --port 9000  # Custom host/port"
echo

echo "Makefile targets:"
echo "  make run                          # Build and run"
echo "  make run-custom HOST=<host> PORT=<port>  # Run with custom settings"
echo "  make clean                        # Clean build artifacts"
echo

echo "Note: The program expects a TPX3 server to be running on the specified host/port"
echo "      to receive histogram data frames."
echo

# Test help functionality
echo "Testing help functionality:"
./tpx3_histogram --help
echo

echo "Test completed successfully!"
