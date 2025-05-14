#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Applying fixes to the webserv server...${NC}"

# Compile the fixes
echo -e "${YELLOW}Compiling fixes...${NC}"
cd src/HttpServer
make
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to compile fixes${NC}"
    exit 1
fi
cd ../..

# Create a backup of the original binary
echo -e "${YELLOW}Creating backup of original binary...${NC}"
if [ -f webserv ]; then
    cp webserv webserv.bak
    echo -e "${GREEN}Backup created: webserv.bak${NC}"
else
    echo -e "${RED}Original binary not found${NC}"
    exit 1
fi

# Recompile the server with our fixes
echo -e "${YELLOW}Recompiling server with fixes...${NC}"
make clean
make
if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to recompile server${NC}"
    echo -e "${YELLOW}Restoring original binary...${NC}"
    cp webserv.bak webserv
    echo -e "${GREEN}Original binary restored${NC}"
    exit 1
fi

echo -e "${GREEN}Fixes applied successfully!${NC}"
echo -e "${YELLOW}You can now run the stress tests:${NC}"
echo -e "cd test/stress_tests && ./run_stress_tests.sh"
echo -e "${YELLOW}Or run the unit tests:${NC}"
echo -e "cd test/unit_tests && make && ./webserv_tests"

exit 0
