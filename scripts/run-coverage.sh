#!/bin/bash
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

COVERAGE_DIR="coverage_report"

# Check for pio command
PIO_CMD="pio"
if ! command -v $PIO_CMD &> /dev/null; then
    if [[ -f "$HOME/.platformio/penv/bin/pio" ]]; then
        PIO_CMD="$HOME/.platformio/penv/bin/pio"
    elif [[ -f "$HOME/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="$HOME/.platformio/penv/Scripts/pio.exe"
    elif [[ -f "$USERPROFILE/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="$USERPROFILE/.platformio/penv/Scripts/pio.exe"
    else
        echo -e "${RED}Error: pio command not found.${NC}"
        exit 1
    fi
fi

echo -e "${BLUE}📊 Running Code Coverage Analysis${NC}"

# Check dependencies
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}Error: lcov is not installed.${NC}"
    echo "Please install it using: sudo apt-get install lcov"
    exit 1
fi

if ! command -v genhtml &> /dev/null; then
    echo -e "${RED}Error: genhtml is not installed.${NC}"
    echo "Please install it using: sudo apt-get install lcov"
    exit 1
fi

# Clean previous build/coverage
echo "Cleaning previous data..."
$PIO_CMD run -e test_coverage -t clean
rm -rf $COVERAGE_DIR

# Run tests
echo "Running unit tests..."
$PIO_CMD test -e test_coverage

# Generate coverage
echo "Generating report..."
mkdir -p $COVERAGE_DIR

# 1. Capture coverage data
# PlatformIO puts build artifacts in .pio/build/test_coverage/
lcov --capture --directory .pio/build/test_coverage --output-file $COVERAGE_DIR/coverage.info --ignore-errors gcov

# 2. Filter data
# Remove system headers, PlatformIO packages, and test files themselves (we want coverage of source code)
lcov --remove $COVERAGE_DIR/coverage.info \
    '/usr/*' \
    '*/.platformio/*' \
    '*/test/*' \
    '*/mock/*' \
    --output-file $COVERAGE_DIR/coverage_filtered.info \
    --ignore-errors unused

# 3. Generate HTML
genhtml $COVERAGE_DIR/coverage_filtered.info \
    --output-directory $COVERAGE_DIR/html \
    --title "HttpClient ESP32 Coverage Report" \
    --legend \
    --show-details

echo -e "${GREEN}✅ Coverage report generated!${NC}"
echo "Open report: $COVERAGE_DIR/html/index.html"

