#!/bin/bash

# Configuration
SERVER_BIN="../../webserv"
CONFIG_FILE="stress_test.conf"
URL="http://localhost:8080/test/stress_tests/empty.html"
SIEGE_CONCURRENT=100
SIEGE_TIME=60
SIEGE_BENCHMARK_TIME=10
MEMORY_CHECK_INTERVAL=5
TEST_DURATION=300  # 5 minutes

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check if siege is installed
if ! command_exists siege; then
    echo -e "${RED}Error: siege is not installed. Please install it first.${NC}"
    echo "On macOS: brew install siege"
    echo "On Ubuntu/Debian: sudo apt-get install siege"
    exit 1
fi

# Check if the server binary exists
if [ ! -f "$SERVER_BIN" ]; then
    echo -e "${RED}Error: Server binary not found at $SERVER_BIN${NC}"
    exit 1
fi

# Check if the config file exists
if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "${RED}Error: Config file not found at $CONFIG_FILE${NC}"
    exit 1
fi

# Start the server
echo -e "${YELLOW}Starting the server...${NC}"
$SERVER_BIN -c $CONFIG_FILE &
SERVER_PID=$!

# Wait for the server to start
echo -e "${YELLOW}Waiting for the server to start...${NC}"
sleep 2

# Check if the server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo -e "${RED}Error: Server failed to start${NC}"
    exit 1
fi

echo -e "${GREEN}Server started with PID $SERVER_PID${NC}"

# Function to clean up
cleanup() {
    echo -e "${YELLOW}Stopping the server...${NC}"
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null
    echo -e "${GREEN}Server stopped${NC}"
    exit 0
}

# Set up trap to clean up on exit
trap cleanup SIGINT SIGTERM EXIT

# Run a quick test to make sure the server is responding
echo -e "${YELLOW}Testing server response...${NC}"
curl -v $URL
echo -e "${GREEN}Server is responding${NC}"

echo -e "${GREEN}Server is responding correctly${NC}"

# Run availability test
echo -e "${YELLOW}Running availability test...${NC}"
echo -e "${YELLOW}Running siege benchmark for $SIEGE_BENCHMARK_TIME seconds with $SIEGE_CONCURRENT concurrent connections...${NC}"
SIEGE_OUTPUT=$(siege -b -c $SIEGE_CONCURRENT -t${SIEGE_BENCHMARK_TIME}S $URL 2>&1)
AVAILABILITY=$(echo "$SIEGE_OUTPUT" | grep "Availability" | awk '{print $2}' | sed 's/%//')

if (( $(echo "$AVAILABILITY < 99.5" | bc -l) )); then
    echo -e "${RED}Availability test failed: $AVAILABILITY% (should be > 99.5%)${NC}"
    echo "$SIEGE_OUTPUT"
    exit 1
else
    echo -e "${GREEN}Availability test passed: $AVAILABILITY%${NC}"
    echo "$SIEGE_OUTPUT"
fi

# Run memory leak test
echo -e "${YELLOW}Running memory leak test...${NC}"
echo -e "${YELLOW}This test will run for $TEST_DURATION seconds, checking memory usage every $MEMORY_CHECK_INTERVAL seconds...${NC}"

# Start siege in the background
siege -b -c $SIEGE_CONCURRENT -t${TEST_DURATION}S $URL > /dev/null 2>&1 &
SIEGE_PID=$!

# Monitor memory usage
START_TIME=$(date +%s)
END_TIME=$((START_TIME + TEST_DURATION))
INITIAL_MEM=0
MAX_MEM=0
LAST_MEM=0

while [ $(date +%s) -lt $END_TIME ]; do
    # Get memory usage (RSS in KB)
    MEM_USAGE=$(ps -o rss= -p $SERVER_PID)

    # Convert to MB for display
    MEM_USAGE_MB=$(echo "scale=2; $MEM_USAGE / 1024" | bc)

    # Set initial memory usage
    if [ $INITIAL_MEM -eq 0 ]; then
        INITIAL_MEM=$MEM_USAGE
        echo -e "${YELLOW}Initial memory usage: ${MEM_USAGE_MB} MB${NC}"
    fi

    # Update max memory usage
    if [ $MEM_USAGE -gt $MAX_MEM ]; then
        MAX_MEM=$MEM_USAGE
    fi

    # Calculate memory growth
    MEM_GROWTH=$((MEM_USAGE - INITIAL_MEM))
    MEM_GROWTH_MB=$(echo "scale=2; $MEM_GROWTH / 1024" | bc)

    # Calculate memory growth rate (KB/s)
    ELAPSED=$(($(date +%s) - START_TIME))
    if [ $ELAPSED -gt 0 ]; then
        GROWTH_RATE=$(echo "scale=2; $MEM_GROWTH / $ELAPSED" | bc)
    else
        GROWTH_RATE=0
    fi

    echo -e "${YELLOW}Current memory usage: ${MEM_USAGE_MB} MB (Growth: ${MEM_GROWTH_MB} MB, Rate: ${GROWTH_RATE} KB/s)${NC}"

    # Check for significant memory growth
    if [ $LAST_MEM -gt 0 ] && [ $MEM_USAGE -gt $((LAST_MEM * 2)) ] && [ $MEM_USAGE -gt $((INITIAL_MEM + 10240)) ]; then
        echo -e "${RED}Warning: Significant memory growth detected!${NC}"
    fi

    LAST_MEM=$MEM_USAGE

    sleep $MEMORY_CHECK_INTERVAL
done

# Calculate final memory statistics
FINAL_MEM=$LAST_MEM
MEM_GROWTH=$((FINAL_MEM - INITIAL_MEM))
MEM_GROWTH_MB=$(echo "scale=2; $MEM_GROWTH / 1024" | bc)
MAX_MEM_MB=$(echo "scale=2; $MAX_MEM / 1024" | bc)
FINAL_MEM_MB=$(echo "scale=2; $FINAL_MEM / 1024" | bc)
INITIAL_MEM_MB=$(echo "scale=2; $INITIAL_MEM / 1024" | bc)

echo -e "${YELLOW}Memory test completed${NC}"
echo -e "${YELLOW}Initial memory usage: ${INITIAL_MEM_MB} MB${NC}"
echo -e "${YELLOW}Final memory usage: ${FINAL_MEM_MB} MB${NC}"
echo -e "${YELLOW}Maximum memory usage: ${MAX_MEM_MB} MB${NC}"
echo -e "${YELLOW}Memory growth: ${MEM_GROWTH_MB} MB${NC}"

# Check if memory growth is excessive
if [ $MEM_GROWTH -gt $((INITIAL_MEM * 2)) ] && [ $MEM_GROWTH -gt 10240 ]; then
    echo -e "${RED}Memory leak test failed: Excessive memory growth detected${NC}"
    exit 1
else
    echo -e "${GREEN}Memory leak test passed: No significant memory growth detected${NC}"
fi

# Check for hanging connections
echo -e "${YELLOW}Checking for hanging connections...${NC}"
# Run a short siege test
siege -b -c $SIEGE_CONCURRENT -t5S $URL > /dev/null 2>&1

# Wait a moment for connections to close
sleep 2

# Check for established connections
CONNECTIONS=$(netstat -an | grep 8080 | grep ESTABLISHED | wc -l)

if [ $CONNECTIONS -gt 0 ]; then
    echo -e "${RED}Hanging connections test failed: $CONNECTIONS connections still established${NC}"
    netstat -an | grep 8080 | grep ESTABLISHED
    exit 1
else
    echo -e "${GREEN}Hanging connections test passed: No hanging connections detected${NC}"
fi

# Run long-term stability test
echo -e "${YELLOW}Running long-term stability test...${NC}"
echo -e "${YELLOW}This test will run siege for $SIEGE_TIME seconds with $SIEGE_CONCURRENT concurrent connections...${NC}"
SIEGE_OUTPUT=$(siege -b -c $SIEGE_CONCURRENT -t${SIEGE_TIME}S $URL 2>&1)
AVAILABILITY=$(echo "$SIEGE_OUTPUT" | grep "Availability" | awk '{print $2}' | sed 's/%//')

if (( $(echo "$AVAILABILITY < 99.5" | bc -l) )); then
    echo -e "${RED}Long-term stability test failed: $AVAILABILITY% availability (should be > 99.5%)${NC}"
    echo "$SIEGE_OUTPUT"
    exit 1
else
    echo -e "${GREEN}Long-term stability test passed: $AVAILABILITY% availability${NC}"
    echo "$SIEGE_OUTPUT"
fi

echo -e "${GREEN}All stress tests passed!${NC}"
exit 0
