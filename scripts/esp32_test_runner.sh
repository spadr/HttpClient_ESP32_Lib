#!/bin/bash

# ESP32 Test Runner with Pre-flight Checks
# Comprehensive script for ESP32 testing with validation and troubleshooting

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Banner
echo -e "${CYAN}"
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║                ESP32 Test Runner v1.0                       ║"
echo "║              HttpClient ESP32 Library                       ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Check if we're in the right directory
if [ ! -f "platformio.ini" ]; then
    echo -e "${RED}Error: platformio.ini not found. Please run from project root.${NC}"
    exit 1
fi

# Function to check requirements
check_requirements() {
    echo -e "${BLUE}🔍 Checking requirements...${NC}"
    
    # Check PlatformIO
    if command -v pio &> /dev/null; then
        echo -e "${GREEN}✓ PlatformIO found: $(pio --version | head -n1)${NC}"
    else
        echo -e "${RED}✗ PlatformIO not found${NC}"
        echo "Install: pip install platformio"
        return 1
    fi
    
    # Check Config.h
    if [ -f "src/Config.h" ]; then
        ssid=$(grep 'const char \*ssid' src/Config.h | cut -d'"' -f2)
        echo -e "${GREEN}✓ WiFi configuration found: SSID='${ssid}'${NC}"
    else
        echo -e "${RED}✗ src/Config.h not found${NC}"
        echo "Please create src/Config.h with WiFi credentials"
        return 1
    fi
    
    # Check test files
    if [ -f "test/main.cpp" ]; then
        test_count=$(grep -c "void test_" test/main.cpp)
        echo -e "${GREEN}✓ Test file found: ${test_count} test functions${NC}"
    else
        echo -e "${RED}✗ test/main.cpp not found${NC}"
        return 1
    fi
    
    return 0
}

# Function to validate test code
validate_tests() {
    echo -e "${BLUE}🧪 Validating test implementations...${NC}"
    
    if [ -f "test/test_validation/validate_esp32_tests.cpp" ]; then
        if g++ -std=c++17 test/test_validation/validate_esp32_tests.cpp -o validate_esp32_tests 2>/dev/null; then
            echo -e "${YELLOW}Running test validation...${NC}"
            if ./validate_esp32_tests; then
                echo -e "${GREEN}✓ All test implementations validated${NC}"
                rm -f validate_esp32_tests
                return 0
            else
                echo -e "${RED}✗ Test validation failed${NC}"
                rm -f validate_esp32_tests
                return 1
            fi
        else
            echo -e "${YELLOW}⚠ Could not compile test validation (missing g++)${NC}"
        fi
    else
        echo -e "${YELLOW}⚠ Test validation file not found${NC}"
    fi
    
    return 0
}

# Function to check if running in WSL and setup USB
check_wsl() {
    if grep -q Microsoft /proc/version &> /dev/null; then
        echo -e "${YELLOW}🐧 Running in WSL environment${NC}"
        echo -e "${BLUE}USB device access requires usbipd-win setup${NC}"
        echo
        
        # Check if USB devices are accessible
        if ! ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null; then
            echo -e "${RED}✗ No USB devices accessible from WSL${NC}"
            echo
            echo -e "${YELLOW}To attach ESP32 device to WSL:${NC}"
            echo "1. First time only (Windows PowerShell as Admin):"
            echo -e "   ${GREEN}usbipd list${NC}"
            echo -e "   ${GREEN}usbipd bind --busid <BUSID>${NC}  # e.g., 4-4"
            echo
            echo "2. Run ESP32 attach script:"
            echo -e "   ${GREEN}./scripts/esp32_attach.sh${NC}"
            echo
            read -p "Run ESP32 attach script now? (Y/n): " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Nn]$ ]]; then
                if [ -f "./scripts/esp32_attach.sh" ]; then
                    ./scripts/esp32_attach.sh
                    echo
                    echo -e "${GREEN}USB setup completed. Continuing with tests...${NC}"
                else
                    echo -e "${RED}Error: esp32_attach.sh not found${NC}"
                    return 1
                fi
            else
                return 1
            fi
        else
            echo -e "${GREEN}✓ USB devices accessible from WSL${NC}"
            ls -la /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
        fi
    fi
    return 0
}

# Function to check devices
check_devices() {
    echo -e "${BLUE}🔌 Checking for ESP32 devices...${NC}"
    
    # Check for devices
    devices=$(pio device list 2>/dev/null || echo "")
    
    if [ -n "$devices" ] && echo "$devices" | grep -q "tty"; then
        echo -e "${GREEN}✓ Devices found:${NC}"
        echo "$devices"
        return 0
    else
        echo -e "${YELLOW}⚠ No ESP32 device detected${NC}"
        echo "Please connect your ESP32 device via USB"
        echo
        echo "Common troubleshooting:"
        echo "  • Check USB cable connection"
        echo "  • Install CP2102/CH340 drivers if needed"
        echo "  • Try different USB port"
        echo "  • Check device permissions (Linux: add user to dialout group)"
        
        # WSL-specific guidance
        if grep -q Microsoft /proc/version &> /dev/null; then
            echo
            echo -e "${YELLOW}WSL-specific:${NC}"
            echo "  • Ensure USB device is attached to WSL using usbipd"
            echo "  • Run: ./scripts/esp32_attach.sh to attach ESP32"
        fi
        
        echo
        read -p "Continue anyway? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            return 1
        fi
    fi
    
    return 0
}

# Function to run tests
run_tests() {
    echo -e "${BLUE}🚀 Running ESP32 tests...${NC}"
    echo
    
    # Try to run tests
    echo -e "${YELLOW}Command: pio test -e m5stack-atom${NC}"
    echo
    
    if pio test -e m5stack-atom; then
        echo
        echo -e "${GREEN}🎉 ESP32 tests completed successfully!${NC}"
        return 0
    else
        echo
        echo -e "${RED}❌ ESP32 tests failed${NC}"
        echo
        echo -e "${YELLOW}Common issues and solutions:${NC}"
        echo "1. Platform installation: pio platform install espressif32"
        echo "2. Device connection: Check USB cable and drivers"
        echo "3. WiFi credentials: Verify src/Config.h settings"
        echo "4. Network connectivity: Test internet connection"
        echo
        return 1
    fi
}

# Function to show post-test options
show_options() {
    echo
    echo -e "${BLUE}📋 Post-test options:${NC}"
    echo "1. Monitor serial output: pio device monitor -e m5stack-atom"
    echo "2. Re-run with verbose: pio test -e m5stack-atom -v"
    echo "3. Build only (no upload): pio run -e m5stack-atom"
    echo "4. Clean build: pio run -e m5stack-atom -t clean"
    echo
}

# Main execution
main() {
    echo -e "${BLUE}Starting ESP32 test execution...${NC}"
    echo
    
    # Step 1: Check requirements
    if ! check_requirements; then
        echo -e "${RED}❌ Requirement check failed${NC}"
        exit 1
    fi
    echo
    
    # Step 2: Check WSL environment
    check_wsl
    echo
    
    # Step 3: Validate tests
    validate_tests
    echo
    
    # Step 4: Check devices
    if ! check_devices; then
        echo -e "${RED}❌ Device check failed${NC}"
        exit 1
    fi
    echo
    
    # Step 4: Run tests
    if run_tests; then
        show_options
        exit 0
    else
        echo -e "${RED}❌ Test execution failed${NC}"
        show_options
        exit 1
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "ESP32 Test Runner"
        echo
        echo "Usage: $0 [options]"
        echo
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --check, -c    Run pre-flight checks only"
        echo "  --validate, -v Validate test code only"
        echo
        echo "Examples:"
        echo "  $0                # Run full test suite"
        echo "  $0 --check       # Check environment only"
        echo "  $0 --validate    # Validate tests only"
        ;;
    --check|-c)
        check_requirements && check_devices
        ;;
    --validate|-v)
        validate_tests
        ;;
    *)
        main
        ;;
esac