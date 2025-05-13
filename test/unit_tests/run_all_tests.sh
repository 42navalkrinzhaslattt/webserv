#!/bin/bash

# Build all tests
make

# Run all tests
echo "Running all tests..."
./webserv_tests

# Clean up
make clean
