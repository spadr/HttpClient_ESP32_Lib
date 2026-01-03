#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'
BOLD='\033[1m'

echo -e "${BOLD}📊 Test Results Summary${NC}"
echo "========================"

process_log() {
    local file=$1
    local name=$2

    if [ -f "$file" ]; then
        echo -e "\n${BLUE}## $name${NC}"
        
        # Clean ANSI codes
        local clean_log=$(sed 's/\x1b\[[0-9;]*m//g' "$file")
        
        # Check summary line
        local summary=$(echo "$clean_log" | grep "test cases:" | tail -n 1)
        
        if [ -n "$summary" ]; then
            # Check for failures in the summary line or "FAILED" entries
            if echo "$clean_log" | grep -q "\[FAILED\]"; then
                echo -e "${RED}❌ Tests Failed${NC}"
                echo -e "${RED}Failed Test Cases:${NC}"
                echo "$clean_log" | grep "\[FAILED\]" | while read -r line; do
                    echo -e "  ${RED}$line${NC}"
                done
            else
                echo -e "${GREEN}✅ Tests Passed${NC}"
            fi
            echo -e "   $summary"
        else
            echo -e "${YELLOW}⚠️  No summary found (Build error?)${NC}"
            echo "   Last 5 lines:"
            tail -n 5 "$file" | sed 's/^/   /'
        fi
    else
        # If file doesn't exist, check if we might want to suppress message or show "Not Run"
        # For now, showing "No result" is fine to indicate it hasn't been run locally yet.
        echo -e "\n${BLUE}## $name${NC}"
        echo -e "   ${YELLOW}No result file found (Not executed yet)${NC}"
    fi
}

process_log ".pio/test/test_result_unit.txt" "Layer 1: Unit Tests"
process_log ".pio/test/test_result_integration.txt" "Layer 2: Integration Tests"
# process_log ".pio/test/test_result_e2e.txt" "Layer 3: E2E Tests" # Uncomment when implemented

echo ""

