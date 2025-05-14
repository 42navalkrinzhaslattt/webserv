# Makefile for stress tests

# Default target
all: check_deps

# Check dependencies
check_deps:
	@echo "Checking dependencies..."
	@which siege > /dev/null 2>&1 || (echo "Error: siege is not installed. Please install it first." && exit 1)
	@which bc > /dev/null 2>&1 || (echo "Error: bc is not installed. Please install it first." && exit 1)
	@which netstat > /dev/null 2>&1 || (echo "Error: netstat is not installed. Please install it first." && exit 1)
	@echo "All dependencies are installed."

# Run stress tests
run: check_deps
	@echo "Running stress tests..."
	@./run_stress_tests.sh

# Clean up
clean:
	@echo "Cleaning up..."
	@rm -f test_config.conf
	@rm -rf test_stress_dir

.PHONY: all check_deps run clean
