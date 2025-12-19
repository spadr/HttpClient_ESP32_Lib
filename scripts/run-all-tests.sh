#!/bin/bash

# HttpClient ESP32 Test Runner
# Usage: ./scripts/run-tests.sh [layer] [options]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default values
LAYER="all"
VERBOSE=""
CLEAN=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -l|--layer)
            LAYER="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE="--verbose"
            shift
            ;;
        -c|--clean)
            CLEAN="yes"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -l, --layer LAYER    Run specific test layer (1, 2, 3, all)"
            echo "  -v, --verbose        Verbose output"
            echo "  -c, --clean          Clean build before testing"
            echo "  -h, --help           Show this help"
            echo ""
            echo "Test Layers:"
            echo "  1  - Unit Tests (Native, fast)"
            echo "  2  - Integration Tests (Native + MockServer)"
            echo "  3  - E2E Tests (ESP32 hardware required)"
            echo "  all - Run all available tests"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}🚀 HttpClient ESP32 Test Runner${NC}"
echo "=================================="

# Clean if requested
if [[ "$CLEAN" == "yes" ]]; then
    echo -e "${YELLOW}🧹 Cleaning build artifacts...${NC}"
    pio run --target clean
    rm -rf .pio/test/
fi

# Function to check if MockServer is running
check_mockserver() {
    if curl -f http://localhost:1080/mockserver/status >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Function to start MockServer
start_mockserver() {
    echo -e "${YELLOW}🐳 Starting MockServer...${NC}"
    docker-compose up -d mockserver
    
    echo "Waiting for MockServer to be ready..."
    for i in {1..30}; do
        if check_mockserver; then
            echo -e "${GREEN}✅ MockServer is ready${NC}"
            return 0
        fi
        sleep 2
    done
    
    echo -e "${RED}❌ MockServer failed to start${NC}"
    return 1
}

# Function to run Layer 1 tests
run_layer1() {
    echo -e "${BLUE}🔬 Running Layer 1: Unit Tests${NC}"
    echo "Target: Native environment (fast)"
    echo "Duration: ~10 seconds"
    echo ""
    
    pio test -e native $VERBOSE
    
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✅ Layer 1 tests passed${NC}"
        return 0
    else
        echo -e "${RED}❌ Layer 1 tests failed${NC}"
        return 1
    fi
}

# Function to run Layer 2 tests
run_layer2() {
    echo -e "${BLUE}🌐 Running Layer 2: Integration Tests${NC}"
    echo "Target: Native + MockServer"
    echo "Duration: ~2 minutes"
    echo ""
    
    # Check if MockServer is running, start if needed
    if ! check_mockserver; then
        start_mockserver || return 1
    else
        echo -e "${GREEN}✅ MockServer already running${NC}"
    fi
    
    # Install system dependencies if needed
    if ! ldconfig -p | grep -q libcurl; then
        echo -e "${YELLOW}📦 Installing libcurl...${NC}"
        if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            sudo apt-get update && sudo apt-get install -y libcurl4-openssl-dev
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            brew install curl
        fi
    fi
    
    echo -e "${BLUE}Running Native Mock Tests...${NC}"
    pio test -e test_native_mock $VERBOSE
    local mock_result=$?

    echo -e "${BLUE}Running Legacy Integration Tests...${NC}"
    pio test -e native_integration $VERBOSE
    local integration_result=$?
    
    if [[ $mock_result -eq 0 && $integration_result -eq 0 ]]; then
        echo -e "${GREEN}✅ Layer 2 tests passed${NC}"
        return 0
    else
        echo -e "${RED}❌ Layer 2 tests failed${NC}"
        return 1
    fi
}

# Function to run Layer 3 tests
run_layer3() {
    echo -e "${BLUE}🔌 Running Layer 3: E2E Tests${NC}"
    echo "Target: ESP32 hardware"
    echo "Duration: ~10 minutes"
    echo ""
    
    # Check if hardware is connected
    if ! pio device list | grep -q "ESP32"; then
        echo -e "${YELLOW}⚠️ No ESP32 device detected${NC}"
        echo "Building tests without execution..."
        pio test -e m5stack-atom --without-testing $VERBOSE
        pio test -e m5stack-atom-ex --without-testing $VERBOSE
        return 0
    fi
    
    echo -e "${YELLOW}🔧 ESP32 device detected, running hardware tests...${NC}"
    pio test -e m5stack-atom $VERBOSE
    
    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✅ Layer 3 tests passed${NC}"
        return 0
    else
        echo -e "${RED}❌ Layer 3 tests failed${NC}"
        return 1
    fi
}

# Main execution
case $LAYER in
    1)
        run_layer1
        ;;
    2)
        run_layer2
        ;;
    3)
        run_layer3
        ;;
    all)
        echo -e "${BLUE}🎯 Running all test layers${NC}"
        echo ""
        
        FAILED_LAYERS=""
        
        if ! run_layer1; then
            FAILED_LAYERS="$FAILED_LAYERS 1"
        fi
        
        echo ""
        if ! run_layer2; then
            FAILED_LAYERS="$FAILED_LAYERS 2"
        fi
        
        echo ""
        if ! run_layer3; then
            FAILED_LAYERS="$FAILED_LAYERS 3"
        fi
        
        echo ""
        echo "=================================="
        if [[ -z "$FAILED_LAYERS" ]]; then
            echo -e "${GREEN}🎉 All tests passed!${NC}"
            echo "✅ Layer 1: Unit Tests"
            echo "✅ Layer 2: Integration Tests"
            echo "✅ Layer 3: E2E Tests"
            exit 0
        else
            echo -e "${RED}❌ Some tests failed${NC}"
            for layer in $FAILED_LAYERS; do
                echo "❌ Layer $layer failed"
            done
            exit 1
        fi
        ;;
    *)
        echo -e "${RED}Unknown layer: $LAYER${NC}"
        echo "Valid layers: 1, 2, 3, all"
        exit 1
        ;;
esac