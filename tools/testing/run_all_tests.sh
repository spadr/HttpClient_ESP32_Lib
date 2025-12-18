#!/bin/bash

# HttpClient_ESP32_Lib 全テスト実行スクリプト
# 作成日: 2025-01-21

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}HttpClient_ESP32_Lib Test Suite${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# テスト結果を保持する変数
UNIT_TEST_RESULT=0
INTEGRATION_TEST_RESULT=0
E2E_TEST_RESULT=0

# 1. 環境チェック
echo -e "${YELLOW}[1/4] 環境チェック...${NC}"

# コンパイラチェック
if ! command -v g++ &> /dev/null; then
    echo -e "${RED}❌ g++が見つかりません。インストールしてください。${NC}"
    echo "実行: sudo apt-get install g++"
    exit 1
fi

# Dockerチェック
if ! command -v docker &> /dev/null; then
    echo -e "${RED}❌ Dockerが見つかりません。インストールしてください。${NC}"
    exit 1
fi

# PlatformIOチェック（オプション）
if command -v pio &> /dev/null; then
    echo -e "${GREEN}✅ PlatformIO: $(pio --version)${NC}"
    USE_PIO=1
else
    echo -e "${YELLOW}⚠️  PlatformIOが見つかりません。手動コンパイルで実行します。${NC}"
    USE_PIO=0
fi

echo ""

# 2. ユニットテスト
echo -e "${YELLOW}[2/4] ユニットテスト実行...${NC}"

if [ "$USE_PIO" = "1" ]; then
    echo "PlatformIOでユニットテストを実行..."
    pio test -e native || UNIT_TEST_RESULT=$?
else
    echo "手動コンパイルでユニットテストを実行..."
    
    # Utilsテスト
    echo -e "${YELLOW}テスト: Utils${NC}"
    if g++ -std=c++17 -DNATIVE_TEST -DARDUINO_ARCH_NATIVE -I src -I test/helpers \
        test/unit/utils/test_utils.cpp src/utils/Utils.cpp src/utils/HttpMethod.cpp \
        src/native_arduino_compat.cpp -o test_utils 2>/dev/null && ./test_utils; then
        echo -e "${GREEN}✅ Utils テスト成功${NC}"
    else
        echo -e "${RED}❌ Utils テスト失敗${NC}"
        UNIT_TEST_RESULT=1
    fi
    
    # Cookieテスト
    echo -e "${YELLOW}テスト: Cookie${NC}"
    if g++ -std=c++17 -DNATIVE_TEST -DARDUINO_ARCH_NATIVE -I src -I test/helpers \
        test/unit/cookie/test_cookie.cpp src/cookie/CookieJar.cpp src/utils/Utils.cpp \
        src/utils/HttpMethod.cpp src/native_arduino_compat.cpp -o test_cookie 2>/dev/null && ./test_cookie; then
        echo -e "${GREEN}✅ Cookie テスト成功${NC}"
    else
        echo -e "${RED}❌ Cookie テスト失敗${NC}"
        UNIT_TEST_RESULT=1
    fi
    
    # Authテスト
    echo -e "${YELLOW}テスト: Auth${NC}"
    if g++ -std=c++17 -DNATIVE_TEST -DARDUINO_ARCH_NATIVE -I src -I test/helpers \
        test/unit/auth/test_auth.cpp src/auth/Auth.cpp src/utils/Utils.cpp \
        src/utils/HttpMethod.cpp src/native_arduino_compat.cpp -o test_auth 2>/dev/null && ./test_auth; then
        echo -e "${GREEN}✅ Auth テスト成功${NC}"
    else
        echo -e "${RED}❌ Auth テスト失敗${NC}"
        UNIT_TEST_RESULT=1
    fi
    
    # RequestValidatorテスト
    echo -e "${YELLOW}テスト: RequestValidator${NC}"
    if g++ -std=c++17 -DNATIVE_TEST -DARDUINO_ARCH_NATIVE -I src -I test/helpers \
        test/unit/core/test_request_validator.cpp src/core/RequestValidator.cpp \
        src/utils/Utils.cpp src/utils/HttpMethod.cpp src/core/Request.cpp \
        src/native_arduino_compat.cpp -o test_request_validator 2>/dev/null && ./test_request_validator; then
        echo -e "${GREEN}✅ RequestValidator テスト成功${NC}"
    else
        echo -e "${RED}❌ RequestValidator テスト失敗${NC}"
        UNIT_TEST_RESULT=1
    fi
fi

echo ""

# 3. 統合テスト（MockServer使用）
echo -e "${YELLOW}[3/4] 統合テスト実行...${NC}"

# MockServerの起動確認
if docker ps | grep -q mockserver; then
    echo -e "${GREEN}✅ MockServerは既に起動しています${NC}"
else
    echo "MockServerを起動中..."
    cd docker
    docker-compose -f test-services.yml up -d mockserver
    cd ..
    
    # MockServerの起動待機
    echo "MockServerの起動を待機中..."
    for i in {1..30}; do
        if curl -s http://localhost:1080/mockserver/status > /dev/null 2>&1; then
            echo -e "${GREEN}✅ MockServerが起動しました${NC}"
            break
        fi
        sleep 1
        echo -n "."
    done
    echo ""
fi

# 統合テストの実行
if [ "$USE_PIO" = "1" ]; then
    echo "PlatformIOで統合テストを実行..."
    pio test -e native_integration || INTEGRATION_TEST_RESULT=$?
else
    echo -e "${YELLOW}⚠️  PlatformIOなしでは統合テストをスキップします${NC}"
    echo "統合テストを実行するにはPlatformIOをインストールしてください"
fi

echo ""

# 4. E2Eテスト（ESP32実機が必要）
echo -e "${YELLOW}[4/4] E2Eテスト${NC}"

if [ "$SKIP_E2E_TESTS" = "1" ]; then
    echo -e "${YELLOW}⚠️  E2Eテストはスキップされました（SKIP_E2E_TESTS=1）${NC}"
elif [ "$USE_PIO" = "1" ] && pio device list | grep -q "USB"; then
    echo "ESP32デバイスが検出されました。E2Eテストを実行..."
    pio test -e m5stack-atom --filter "e2e/*" || E2E_TEST_RESULT=$?
else
    echo -e "${YELLOW}⚠️  ESP32デバイスが接続されていないため、E2Eテストをスキップします${NC}"
fi

echo ""

# 結果サマリー
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}テスト結果サマリー${NC}"
echo -e "${GREEN}========================================${NC}"

if [ $UNIT_TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ ユニットテスト: 成功${NC}"
else
    echo -e "${RED}❌ ユニットテスト: 失敗${NC}"
fi

if [ $INTEGRATION_TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ 統合テスト: 成功${NC}"
else
    echo -e "${RED}❌ 統合テスト: 失敗${NC}"
fi

if [ "$SKIP_E2E_TESTS" = "1" ] || [ "$USE_PIO" = "0" ]; then
    echo -e "${YELLOW}⏭️  E2Eテスト: スキップ${NC}"
elif [ $E2E_TEST_RESULT -eq 0 ]; then
    echo -e "${GREEN}✅ E2Eテスト: 成功${NC}"
else
    echo -e "${RED}❌ E2Eテスト: 失敗${NC}"
fi

echo ""

# クリーンアップ
echo "テスト用ファイルをクリーンアップ中..."
rm -f test_utils test_cookie test_auth test_request_validator test_integration 2>/dev/null || true

# 総合結果
TOTAL_RESULT=$((UNIT_TEST_RESULT + INTEGRATION_TEST_RESULT + E2E_TEST_RESULT))
if [ $TOTAL_RESULT -eq 0 ]; then
    echo -e "${GREEN}🎉 全テスト成功！${NC}"
    exit 0
else
    echo -e "${RED}❌ 一部のテストが失敗しました${NC}"
    exit 1
fi