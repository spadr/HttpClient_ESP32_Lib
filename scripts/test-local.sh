#!/bin/bash

set -e

echo "🚀 Starting local test suite..."

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
timeout 30 bash -c 'until curl -f http://localhost:1080/mockserver/status > /dev/null 2>&1; do sleep 2; done' || {
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
pio test -e native --verbose

# 統合テストの実行
echo "🔗 Running integration tests..."
pio test -e native_integration --verbose

# E2Eテストの実行
if [ "$1" = "--record" ]; then
    echo "📹 Running E2E tests in recording mode..."
    RECORD_MODE=1 pio test -e native --filter "e2e/*" --verbose
else
    echo "▶️ Running E2E tests in playback mode..."
    pio test -e native --filter "e2e/*" --verbose
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