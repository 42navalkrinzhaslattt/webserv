# Webserv Stress Tests

This directory contains stress tests for the webserv HTTP server. These tests use Siege to simulate high load and verify that the server meets performance requirements.

## Requirements

- Siege: A HTTP load testing and benchmarking utility
- bc: A command-line calculator
- netstat: A command-line network statistics utility

## Installation

### macOS

```bash
brew install siege
```

### Ubuntu/Debian

```bash
sudo apt-get install siege bc net-tools
```

## Running the Tests

To run the stress tests, execute the following command from the project root directory:

```bash
./test/stress_tests/run_stress_tests.sh
```

This script will:

1. Start the webserv server with a test configuration
2. Run a series of stress tests using Siege
3. Monitor memory usage to detect leaks
4. Check for hanging connections
5. Verify long-term stability

## Test Criteria

The stress tests verify that the server meets the following criteria:

1. **Availability**: Above 99.5% for a simple GET on an empty page with siege -b
2. **Memory Usage**: No memory leaks (memory usage should not increase indefinitely)
3. **Connection Handling**: No hanging connections after requests complete
4. **Stability**: Can run indefinitely without requiring a restart

## Unit Tests

In addition to the shell script, there are also unit tests for stress testing in the `test/unit_tests` directory. These tests can be run with:

```bash
cd test/unit_tests
make
./webserv_tests "[stress]"
```

Note that the stress unit tests are tagged with `[.][stress]`, which means they are hidden by default. You need to explicitly specify the `[stress]` tag to run them.

## Customizing the Tests

You can customize the stress tests by modifying the variables at the top of the `run_stress_tests.sh` script:

- `SERVER_BIN`: Path to the webserv binary
- `CONFIG_FILE`: Path to the configuration file
- `URL`: URL to test
- `SIEGE_CONCURRENT`: Number of concurrent connections
- `SIEGE_TIME`: Duration of the long-term stability test (in seconds)
- `SIEGE_BENCHMARK_TIME`: Duration of the availability test (in seconds)
- `MEMORY_CHECK_INTERVAL`: Interval for checking memory usage (in seconds)
- `TEST_DURATION`: Total duration of the memory leak test (in seconds)

## Interpreting the Results

The script will output detailed information about each test, including:

- Availability percentage
- Memory usage statistics
- Number of hanging connections
- Siege statistics

If all tests pass, you'll see a "All stress tests passed!" message at the end. If any test fails, the script will exit with a non-zero status code and display an error message.
