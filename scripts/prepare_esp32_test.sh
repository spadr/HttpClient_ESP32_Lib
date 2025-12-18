#!/bin/bash

# ESP32 Test Preparation Script

set -e

echo "=== ESP32 Test Environment Preparation ==="
echo

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    echo -e "${RED}Error: platformio.ini not found. Please run from project root.${NC}"
    exit 1
fi

echo -e "${BLUE}1. Checking PlatformIO installation...${NC}"
if command -v pio &> /dev/null; then
    echo -e "${GREEN}✓ PlatformIO found${NC}"
    pio --version
else
    echo -e "${RED}✗ PlatformIO not found${NC}"
    echo "Please install PlatformIO: pip install platformio"
    exit 1
fi

echo
echo -e "${BLUE}2. Checking ESP32 devices...${NC}"
if pio device list | grep -q "tty"; then
    echo -e "${GREEN}✓ Devices found:${NC}"
    pio device list
else
    echo -e "${YELLOW}⚠ No ESP32 device detected${NC}"
    echo "Please connect your ESP32 device and ensure drivers are installed."
    echo "Common locations:"
    echo "  - Linux: /dev/ttyUSB* or /dev/ttyACM*"
    echo "  - Windows: COM* ports"
    echo "  - macOS: /dev/cu.usbserial*"
fi

echo
echo -e "${BLUE}3. Checking project configuration...${NC}"

# Check Config.h
if [ -f "src/Config.h" ]; then
    echo -e "${GREEN}✓ src/Config.h found${NC}"
    echo "WiFi SSID: $(grep 'const char \*ssid' src/Config.h | cut -d'"' -f2)"
else
    echo -e "${RED}✗ src/Config.h not found${NC}"
    echo "Please create src/Config.h with your WiFi credentials"
fi

# Check test files
if [ -f "test/main.cpp" ]; then
    echo -e "${GREEN}✓ test/main.cpp found${NC}"
    test_count=$(grep -c "void test_" test/main.cpp)
    echo "Test functions found: $test_count"
else
    echo -e "${RED}✗ test/main.cpp not found${NC}"
fi

echo
echo -e "${BLUE}4. Attempting to compile (build only)...${NC}"
if pio run -e m5stack-atom; then
    echo -e "${GREEN}✓ Compilation successful${NC}"
    echo
    echo -e "${BLUE}5. Ready for testing!${NC}"
    echo "To run tests on connected ESP32:"
    echo "  pio test -e m5stack-atom"
    echo
    echo "To monitor serial output:"
    echo "  pio device monitor -e m5stack-atom"
else
    echo -e "${RED}✗ Compilation failed${NC}"
    echo "Please fix compilation errors before running tests"
    exit 1
fi

echo
echo -e "${YELLOW}=== Test Execution Instructions ===${NC}"
echo "1. Connect ESP32 device via USB"
echo "2. Update WiFi credentials in src/Config.h"
echo "3. Run: pio test -e m5stack-atom"
echo "4. Monitor: pio device monitor -e m5stack-atom"
echo
echo -e "${GREEN}ESP32 test environment prepared successfully!${NC}"