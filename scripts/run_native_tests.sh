#!/bin/bash

# Native Tests Runner Script for HttpClient ESP32 Library

set -e  # Exit on any error

echo "=== HttpClient ESP32 Library - Native Test Runner ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_command=$2
    
    echo -e "${BLUE}Running: ${test_name}${NC}"
    echo "Command: $test_command"
    
    if eval $test_command; then
        echo -e "${GREEN}✓ PASSED: ${test_name}${NC}"
        ((PASSED_TESTS++))
    else
        echo -e "${RED}✗ FAILED: ${test_name}${NC}"
        ((FAILED_TESTS++))
    fi
    ((TOTAL_TESTS++))
    echo
}

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    echo -e "${RED}Error: platformio.ini not found. Please run from project root.${NC}"
    exit 1
fi

# Create logs directory
mkdir -p logs

echo -e "${YELLOW}=== Layer 1: Unit Tests (Native) ===${NC}"

# Utils Tests
run_test "Utils Library Tests" "g++ -std=c++17 -DNATIVE_TEST -DARDUINO_ARCH_NATIVE -I./src -I./test/helpers ./test/unit/utils/test_utils.cpp ./src/utils/Utils.cpp ./src/utils/HttpMethod.cpp ./src/native_arduino_compat.cpp ./test/unit/native_arduino_mock.cpp -o ./test_utils_runner && ./test_utils_runner"

# Auth Tests (if compiler available)
if command -v g++ &> /dev/null; then
    echo -e "${BLUE}Compiler found: g++${NC}"
else
    echo -e "${YELLOW}Warning: g++ compiler not found, skipping native compilation tests${NC}"
fi

# Try to run with PlatformIO if available
if command -v pio &> /dev/null; then
    echo -e "${BLUE}PlatformIO found, running tests...${NC}"
    run_test "PlatformIO Unit Tests" "pio test -e native --verbose"
    run_test "PlatformIO Integration Tests" "pio test -e native_integration --verbose"
else
    echo -e "${YELLOW}Warning: PlatformIO not found in PATH${NC}"
fi

echo -e "${YELLOW}=== Test Summary ===${NC}"
echo -e "Total Tests: ${TOTAL_TESTS}"
echo -e "Passed: ${GREEN}${PASSED_TESTS}${NC}"
echo -e "Failed: ${RED}${FAILED_TESTS}${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Check the output above.${NC}"
    exit 1
fi