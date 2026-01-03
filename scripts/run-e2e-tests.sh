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
echo "Target: ESP32 hardware (test_e2e)"

# Check for pio command
PIO_CMD="pio"
if ! command -v $PIO_CMD &> /dev/null; then
    # Try common paths
    # Windows paths for Git Bash / MSYS2 / WSL
    # WSL環境でもWindows側のPIOを使うことで、Windowsに接続されたデバイスを利用可能にする
    if [[ -f "/mnt/c/Users/ennbu/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="/mnt/c/Users/ennbu/.platformio/penv/Scripts/pio.exe"
    elif [[ -f "/c/Users/$USERNAME/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="/c/Users/$USERNAME/.platformio/penv/Scripts/pio.exe"
    elif [[ -f "/c/Users/${USER}/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="/c/Users/${USER}/.platformio/penv/Scripts/pio.exe"
    # Specific fallback for this environment
    elif [[ -f "/c/Users/ennbu/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="/c/Users/ennbu/.platformio/penv/Scripts/pio.exe"
    # Linux native paths (lower priority to prefer Windows PIO on WSL if available)
    elif [[ -f "$HOME/.platformio/penv/bin/pio" ]]; then
        PIO_CMD="$HOME/.platformio/penv/bin/pio"
    elif [[ -f "$HOME/.platformio/penv/Scripts/pio.exe" ]]; then
        PIO_CMD="$HOME/.platformio/penv/Scripts/pio.exe"
    else
        echo -e "${RED}Error: pio command not found.${NC}"
        echo "Please ensure PlatformIO is installed and in your PATH."
        exit 1
    fi
fi

echo "Debug: PIO_CMD set to '$PIO_CMD'"

if [[ "$CLEAN" == "yes" ]]; then
    echo -e "${YELLOW}🧹 Cleaning build artifacts...${NC}"
    $PIO_CMD run --target clean
    rm -rf .pio/test/
fi

# Check if hardware is connected
echo "Checking connected devices..."
# 変数経由での実行が不安定なため、eval を使用するか、直接実行を試みる
# ここでは変数の展開を確実にするため eval を使用する

eval "$PIO_CMD device list" > .device_list.tmp 2>&1
EXIT_CODE=$?

DEVICE_LIST=$(cat .device_list.tmp)
rm -f .device_list.tmp

if [ $EXIT_CODE -ne 0 ]; then
    echo -e "${RED}Error running pio device list (Exit code: $EXIT_CODE)${NC}"
    echo "$DEVICE_LIST"
    exit 1
fi

echo "Device List Output:"
echo "$DEVICE_LIST"

UPLOAD_PORT=""
# WindowsのCOMポート検出を強化
if echo "$DEVICE_LIST" | grep -q "ESP32"; then
    echo -e "${GREEN}✅ ESP32 device detected!${NC}"
    # COMポートを抽出して自動設定する（単純な grep なので改善の余地あり）
    UPLOAD_PORT=$(echo "$DEVICE_LIST" | grep -B 3 "ESP32" | grep "COM" | head -n 1 | tr -d '[:space:]')
elif echo "$DEVICE_LIST" | grep -qE "USB Serial|CP210|CH340|FTDI"; then
     echo -e "${YELLOW}⚠️ Generic USB Serial device detected. Assuming it's ESP32...${NC}"
     UPLOAD_PORT=$(echo "$DEVICE_LIST" | grep -B 3 -E "USB Serial|CP210|CH340|FTDI" | grep "COM" | head -n 1 | tr -d '[:space:]')
elif echo "$DEVICE_LIST" | grep -q "COM[0-9]"; then
     echo -e "${YELLOW}⚠️ COM port detected. Attempting to run...${NC}"
     # USB Serial Portを優先的に探す
     UPLOAD_PORT=$(echo "$DEVICE_LIST" | grep -B 3 "USB Serial" | grep "COM" | head -n 1 | tr -d '[:space:]')
     # なければ最初のCOMポート
     if [ -z "$UPLOAD_PORT" ]; then
        UPLOAD_PORT=$(echo "$DEVICE_LIST" | grep "COM" | head -n 1 | tr -d '[:space:]')
     fi
else
    echo -e "${RED}❌ No likely serial devices found.${NC}"
    # 強制実行を許可する場合はここをコメントアウト
    # exit 1 
fi

if [ -n "$UPLOAD_PORT" ]; then
    echo -e "${GREEN}Using upload port: $UPLOAD_PORT${NC}"
    UPLOAD_PORT_FLAG="--upload-port $UPLOAD_PORT"
else
    UPLOAD_PORT_FLAG=""
fi

echo -e "${YELLOW}🔧 Running hardware tests...${NC}"
# 環境名は platformio.ini の [env:test_esp32] ではなく [env:test_e2e] に変更された
$PIO_CMD test -e test_e2e $UPLOAD_PORT_FLAG $VERBOSE

if [[ $? -eq 0 ]]; then
    echo -e "${GREEN}✅ E2E tests passed${NC}"
    exit 0
else
    echo -e "${RED}❌ E2E tests failed${NC}"
    exit 1
fi
