#!/bin/bash

set -e

echo "🚀 Starting local test suite..."

# Find pio executable
find_pio() {
    if command -v pio &> /dev/null; then
        echo "pio"
        return 0
    fi

    # Check common Windows/Unix paths
    # Windows User Profile usually in /c/Users/username or /mnt/c/Users/username in bash
    local user_profile_win="/c/Users/$USERNAME"
    # Fallback to HOME if USERNAME not set or path invalid
    if [ -z "$USERNAME" ] || [ ! -d "$user_profile_win" ]; then
        user_profile_win="$HOME"
    fi

    local paths=(
        "$HOME/.platformio/penv/Scripts/pio.exe"
        "$HOME/.platformio/penv/bin/pio"
        "$user_profile_win/.platformio/penv/Scripts/pio.exe"
        "/c/Users/$USERNAME/.platformio/penv/Scripts/pio.exe"
    )

    for path in "${paths[@]}"; do
        if [ -f "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    
    return 1
}

PIO_CMD=$(find_pio)

if [ -z "$PIO_CMD" ]; then
    echo "❌ PlatformIO (pio) not found in PATH or standard locations."
    echo "Please ensure PlatformIO is installed and available."
    exit 1
fi

echo "✅ Using PlatformIO command: $PIO_CMD"

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker and try again."
    exit 1
fi

# テスト環境の起動
echo "📦 Starting mock services..."
cd docker
docker-compose -f test-services.yml up -d
sleep 10

# サービスの健全性チェック
echo "🔍 Checking service health..."
# MockServerはデフォルトで404を返すことがあるため、-fオプションを外して接続確認のみ行う
timeout 30 bash -c 'until curl -s http://localhost:1080/mockserver/status > /dev/null; do sleep 2; done' || {
    echo "❌ MockServer not ready"
    docker-compose -f test-services.yml logs mockserver
    docker-compose -f test-services.yml down
    exit 1
}

timeout 30 bash -c 'until curl -f http://localhost:8080/__admin/ > /dev/null 2>&1; do sleep 2; done' || {
    echo "❌ WireMock not ready"
    docker-compose -f test-services.yml logs wiremock
    docker-compose -f test-services.yml down
    exit 1
}

echo "✅ All services are ready"

cd ..

# ユニットテストの実行
echo "🧪 Running unit tests..."
"$PIO_CMD" test -e native --verbose

# Integration Tests (Native)
echo "🔗 Running native integration tests..."
"$PIO_CMD" test -e test_integration --verbose

# E2Eテストの実行
if [ "$1" = "--record" ]; then
    echo "📹 Running E2E tests in recording mode..."
    RECORD_MODE=1 "$PIO_CMD" test -e native --filter "e2e/*" --verbose
else
    echo "▶️ Running E2E tests in playback mode..."
    "$PIO_CMD" test -e native --filter "e2e/*" --verbose
fi

# テスト結果のサマリー
echo ""
echo "📊 Test Summary:"
echo "  ✅ Unit Tests: Completed"
echo "  ✅ Integration Tests: Completed"
echo "  ✅ E2E Tests: Completed"

# クリーンアップ
echo "🧹 Cleaning up..."
cd docker
docker-compose -f test-services.yml down

echo "✅ All tests completed successfully!"
