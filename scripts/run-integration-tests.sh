#!/bin/bash
set -e
set -o pipefail

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

echo -e "${BLUE}🌐 Running Integration Tests${NC}"
echo "Target: Native + MockServer (test_integration)"

# Function to check if MockServer is running
check_mockserver() {
    if curl -s -X PUT http://localhost:1080/mockserver/status >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Function to check for docker compose command
get_compose_cmd() {
    if command -v docker-compose &> /dev/null; then
        echo "docker-compose"
    elif docker compose version &> /dev/null; then
        echo "docker compose"
    else
        echo ""
    fi
}

# Function to start MockServer
start_mockserver() {
    COMPOSE_CMD=$(get_compose_cmd)
    
    if [ -z "$COMPOSE_CMD" ]; then
        echo -e "${RED}Error: neither 'docker-compose' nor 'docker compose' found.${NC}"
        echo "Please install Docker Compose."
        return 1
    fi

    echo -e "${YELLOW}🐳 Starting MockServer using $COMPOSE_CMD...${NC}"
    $COMPOSE_CMD -f docker/test-services.yml up -d mockserver
    
    echo "Waiting for MockServer to be ready..."
    for i in {1..30}; do
        if check_mockserver; then
            echo -e "${GREEN}✅ MockServer is ready${NC}"
            return 0
        fi
        sleep 2
    done
    
    echo -e "${RED}❌ MockServer failed to start${NC}"
    
    # Try to clean up and restart once
    echo -e "${YELLOW}🔄 Attempting to cleanup and restart...${NC}"
    $COMPOSE_CMD -f docker/test-services.yml down
    $COMPOSE_CMD -f docker/test-services.yml up -d mockserver
    
    echo "Waiting for MockServer to restart..."
    for i in {1..30}; do
        if check_mockserver; then
            echo -e "${GREEN}✅ MockServer is ready after restart${NC}"
            return 0
        fi
        sleep 2
    done
    
    echo -e "${RED}❌ MockServer failed to start even after cleanup${NC}"
    return 1
}

# Check/Start MockServer
if ! check_mockserver; then
    start_mockserver || exit 1
else
    echo -e "${GREEN}✅ MockServer already running${NC}"
fi

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

# Run tests using the dedicated integration test environment
echo -e "${BLUE}Running tests...${NC}"

# Ensure output directory exists
mkdir -p .pio/test

# Run tests and save output to file
$PIO_CMD test -e test_integration $VERBOSE 2>&1 | tee .pio/test/test_result.txt

if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}✅ Integration tests passed${NC}"
    exit 0
else
    echo -e "${RED}❌ Integration tests failed${NC}"
    exit 1
fi

