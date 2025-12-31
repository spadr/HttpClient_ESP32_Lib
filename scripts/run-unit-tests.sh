#!/bin/bash
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
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

echo -e "${BLUE}🔬 Running Unit Tests${NC}"
echo "Target: Native environment (test_unit)"

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

# Run tests using the dedicated unit test environment
$PIO_CMD test -e test_unit $VERBOSE

if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}✅ Unit tests passed${NC}"
    exit 0
else
    echo -e "${RED}❌ Unit tests failed${NC}"
    exit 1
fi

