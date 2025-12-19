#!/bin/bash
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

VERBOSE=""
CLEAN=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE="--verbose"
            shift
            ;;
        -c|--clean)
            CLEAN="yes"
            shift
            ;;
        *)
            shift
            ;;
    esac
done

echo -e "${BLUE}🔌 Running E2E Tests (ESP32)${NC}"
echo "Target: ESP32 hardware (test_esp32)"

# Check for pio command
PIO_CMD="pio"
if ! command -v $PIO_CMD &> /dev/null; then
    # Try common paths
    if [[ -f "$HOME/.platformio/penv/bin/pio" ]]; then
        PIO_CMD="$HOME/.platformio/penv/bin/pio"
    elif [[ -f "$HOME/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="$HOME/.platformio/penv/Scripts/pio.exe"
    elif [[ -f "$USERPROFILE/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="$USERPROFILE/.platformio/penv/Scripts/pio.exe"
    else
        echo -e "${RED}Error: pio command not found.${NC}"
        echo "Please ensure PlatformIO is installed and in your PATH."
        exit 1
    fi
fi

if [[ "$CLEAN" == "yes" ]]; then
    echo -e "${YELLOW}🧹 Cleaning build artifacts...${NC}"
    $PIO_CMD run --target clean
    rm -rf .pio/test/
fi

# Check if hardware is connected
if ! $PIO_CMD device list | grep -q "ESP32"; then
    echo -e "${YELLOW}⚠️ No ESP32 device detected${NC}"
    echo "Building tests without execution..."
    $PIO_CMD test -e m5stack-atom --without-testing $VERBOSE
    $PIO_CMD test -e m5stack-atom-ex --without-testing $VERBOSE
    exit 0
fi

echo -e "${YELLOW}🔧 ESP32 device detected, running hardware tests...${NC}"
$PIO_CMD test -e m5stack-atom $VERBOSE

if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}✅ E2E tests passed${NC}"
    exit 0
else
    echo -e "${RED}❌ E2E tests failed${NC}"
    exit 1
fi

