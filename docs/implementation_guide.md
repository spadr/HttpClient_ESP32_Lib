# テスト戦略実装ガイド

## 概要

新しいテスト戦略の具体的な実装手順を説明します。段階的に移行することで、既存の開発フローを維持しながら改善を進めます。

## 前提条件

- PlatformIO CLI がインストール済み
- Docker & Docker Compose がインストール済み
- Git がインストール済み

## Phase 1: Native環境構築

### 1.1 PlatformIO設定の更新

```ini
# platformio.ini に追加
[env:native]
platform = native
test_build_src = yes
build_flags = 
    -std=c++17
    -DNATIVE_TEST
    -DUNITY_INCLUDE_DOUBLE
    -Itest/helpers
    -Itest/fixtures
lib_deps =
    throwtheswitch/Unity@^2.5.2
test_filter = unit/*
test_ignore = 
    test/ActualConnectionTest.cpp
    test/*Test.cpp
    
[env:native-integration]
extends = env:native
test_filter = integration/*
lib_deps = 
    ${env:native.lib_deps}
    cpp-httplib@^0.14.0
    nlohmann/json@^3.11.0
```

### 1.2 テストディレクトリ構造の作成

```bash
#!/bin/bash
# scripts/setup-test-structure.sh

# テストディレクトリの作成
mkdir -p test/unit/{mock,auth,cookie,utils}
mkdir -p test/integration/{http_basic,https_ssl,proxy,auth}
mkdir -p test/e2e/{recordings,real_servers,performance}
mkdir -p test/fixtures/{mockserver,certificates,responses}
mkdir -p test/helpers

# 既存テストの移動
mv test/*Test.cpp test/unit/
mv test/*Test.h test/unit/

echo "Test structure created successfully"
```

### 1.3 CI/CD設定の更新

```yaml
# .github/workflows/test-matrix.yml
name: Test Matrix

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Cache PlatformIO
      uses: actions/cache@v3
      with:
        path: ~/.platformio
        key: ${{ runner.os }}-pio-${{ hashFiles('**/platformio.ini') }}
        
    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.9'
        
    - name: Install PlatformIO
      run: |
        pip install --upgrade platformio
        
    - name: Run Unit Tests
      run: pio test -e native
      
  integration-tests:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Start Mock Services
      run: |
        cd docker
        docker-compose up -d
        sleep 10
        
    - name: Setup PlatformIO
      run: |
        pip install --upgrade platformio
        
    - name: Run Integration Tests
      run: pio test -e native-integration
      
    - name: Stop Mock Services
      if: always()
      run: |
        cd docker
        docker-compose down
        
  build-tests:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Setup PlatformIO
      run: |
        pip install --upgrade platformio
        
    - name: Build for ESP32
      run: pio run -e m5stack-atom-ex
```

## Phase 2: MockServer統合

### 2.1 Docker環境の構築

```yaml
# docker/test-services.yml
version: '3.8'
services:
  mockserver:
    image: mockserver/mockserver:5.15.0
    ports:
      - "1080:1080"
    environment:
      MOCKSERVER_INITIALIZATION_JSON_PATH: /config/expectations.json
      MOCKSERVER_LOG_LEVEL: INFO
    volumes:
      - ../test/fixtures/mockserver:/config

  wiremock:
    image: wiremock/wiremock:2.35.0
    ports:
      - "8080:8080"
    command: >
      --global-response-templating
      --verbose
    volumes:
      - ../test/fixtures/wiremock/__files:/home/wiremock/__files
      - ../test/fixtures/wiremock/mappings:/home/wiremock/mappings

  proxy:
    image: ubuntu/squid:latest
    ports:
      - "3128:3128"
    volumes:
      - ../test/fixtures/proxy/squid.conf:/etc/squid/squid.conf
```

### 2.2 MockServer設定ファイル

```json
# test/fixtures/mockserver/expectations.json
[
  {
    "httpRequest": {
      "method": "GET",
      "path": "/api/test"
    },
    "httpResponse": {
      "statusCode": 200,
      "headers": {
        "Content-Type": ["application/json"]
      },
      "body": {
        "message": "Hello from MockServer",
        "timestamp": "2024-01-01T00:00:00Z"
      }
    }
  },
  {
    "httpRequest": {
      "method": "POST",
      "path": "/api/auth",
      "headers": {
        "Authorization": ["Bearer valid-token"]
      }
    },
    "httpResponse": {
      "statusCode": 200,
      "body": {
        "status": "authenticated",
        "user": "test@example.com"
      }
    }
  },
  {
    "httpRequest": {
      "method": "GET",
      "path": "/api/timeout"
    },
    "httpResponse": {
      "statusCode": 200,
      "delay": {
        "timeUnit": "SECONDS",
        "value": 5
      },
      "body": "Delayed response"
    }
  }
]
```

### 2.3 Native統合テストの実装

```cpp
// test/integration/http_basic/BasicHttpTest.cpp
#include <unity.h>
#include "HttpClient.h"
#include "TestEnvironment.h"

void setUp() {
    // テスト前の初期化
}

void tearDown() {
    // テスト後のクリーンアップ
}

void test_mockserver_get_request() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    canaspad::ClientOptions options;
    options.verifySsl = false;
    canaspad::HttpClient client(options, false);
    
    canaspad::Request request;
    request.setUrl("http://localhost:1080/api/test")
           .setMethod(canaspad::HttpMethod::GET);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
    TEST_ASSERT_TRUE(result.value().body.find("Hello from MockServer") != std::string::npos);
}

void test_bearer_authentication() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    canaspad::ClientOptions options;
    options.authType = canaspad::AuthType::Bearer;
    options.bearerToken = "valid-token";
    canaspad::HttpClient client(options, false);
    
    canaspad::Request request;
    request.setUrl("http://localhost:1080/api/auth")
           .setMethod(canaspad::HttpMethod::POST);
    
    auto result = client.send(request);
    
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
}

void test_timeout_handling() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    canaspad::ClientOptions options;
    canaspad::HttpClient client(options, false);
    client.setReadTimeout(std::chrono::seconds(2)); // 2秒でタイムアウト
    
    canaspad::Request request;
    request.setUrl("http://localhost:1080/api/timeout"); // 5秒遅延
    
    auto result = client.send(request);
    
    TEST_ASSERT_FALSE(result.isSuccess());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(canaspad::ErrorCode::Timeout), 
                         static_cast<int>(result.error().code));
}

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_mockserver_get_request);
    RUN_TEST(test_bearer_authentication);
    RUN_TEST(test_timeout_handling);
    
    return UNITY_END();
}
```

### 2.4 テスト環境検出ヘルパー

```cpp
// test/helpers/TestEnvironment.h
#pragma once
#include <string>

class TestEnvironment {
public:
    static bool isMockServerAvailable() {
        return checkConnection("localhost", 1080);
    }
    
    static bool isWireMockAvailable() {
        return checkConnection("localhost", 8080);
    }
    
    static bool isCI() {
        return getenv("CI") != nullptr;
    }
    
    static bool isRecordingMode() {
        return !isCI() && getenv("RECORD_MODE") != nullptr;
    }
    
private:
    static bool checkConnection(const std::string& host, int port);
};
```

```cpp
// test/helpers/TestEnvironment.cpp
#include "TestEnvironment.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

bool TestEnvironment::checkConnection(const std::string& host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    bool result = connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0;
    close(sock);
    return result;
}
```

## Phase 3: Recording Proxy実装

### 3.1 HttpRecorderクラスの実装

```cpp
// test/helpers/HttpRecorder.h
#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include "core/Request.h"
#include "core/HttpResult.h"

namespace canaspad {

struct RecordedInteraction {
    Request request;
    HttpResult response;
    std::chrono::system_clock::time_point timestamp;
    std::string metadata;
};

class HttpRecorder {
public:
    explicit HttpRecorder(const std::string& cassetteDir = "test/e2e/recordings");
    
    // 記録開始
    void startRecording(const std::string& cassetteName);
    
    // 再生開始
    void startPlayback(const std::string& cassetteName);
    
    // 記録停止
    void stopRecording();
    
    // インタラクションを記録
    void recordInteraction(const Request& request, const HttpResult& response);
    
    // 記録されたレスポンスを取得
    std::optional<HttpResult> getRecordedResponse(const Request& request);
    
    // モード判定
    bool isRecordingMode() const { return m_isRecording; }
    bool isPlaybackMode() const { return m_isPlayback; }
    
private:
    std::string m_cassetteDir;
    std::string m_currentCassette;
    bool m_isRecording = false;
    bool m_isPlayback = false;
    
    std::vector<RecordedInteraction> m_interactions;
    
    void saveCassette();
    void loadCassette();
    std::string generateRequestKey(const Request& request);
};

} // namespace canaspad
```

### 3.2 記録再生の実装

```cpp
// test/helpers/HttpRecorder.cpp
#include "HttpRecorder.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace canaspad {

HttpRecorder::HttpRecorder(const std::string& cassetteDir) 
    : m_cassetteDir(cassetteDir) {
    fs::create_directories(cassetteDir);
}

void HttpRecorder::startRecording(const std::string& cassetteName) {
    m_currentCassette = cassetteName;
    m_isRecording = true;
    m_isPlayback = false;
    m_interactions.clear();
}

void HttpRecorder::startPlayback(const std::string& cassetteName) {
    m_currentCassette = cassetteName;
    m_isRecording = false;
    m_isPlayback = true;
    loadCassette();
}

void HttpRecorder::recordInteraction(const Request& request, const HttpResult& response) {
    if (!m_isRecording) return;
    
    RecordedInteraction interaction;
    interaction.request = request;
    interaction.response = response;
    interaction.timestamp = std::chrono::system_clock::now();
    
    m_interactions.push_back(interaction);
}

std::optional<HttpResult> HttpRecorder::getRecordedResponse(const Request& request) {
    if (!m_isPlayback) return std::nullopt;
    
    std::string requestKey = generateRequestKey(request);
    
    for (const auto& interaction : m_interactions) {
        if (generateRequestKey(interaction.request) == requestKey) {
            return interaction.response;
        }
    }
    
    return std::nullopt;
}

void HttpRecorder::saveCassette() {
    json cassetteJson;
    cassetteJson["version"] = "1.0";
    cassetteJson["interactions"] = json::array();
    
    for (const auto& interaction : m_interactions) {
        json interactionJson;
        
        // Request
        interactionJson["request"]["method"] = static_cast<int>(interaction.request.getMethod());
        interactionJson["request"]["url"] = interaction.request.getUrl();
        interactionJson["request"]["headers"] = interaction.request.getHeaders();
        interactionJson["request"]["body"] = interaction.request.getBody();
        
        // Response
        interactionJson["response"]["status_code"] = interaction.response.statusCode;
        interactionJson["response"]["headers"] = interaction.response.headers;
        interactionJson["response"]["body"] = interaction.response.body;
        
        // Metadata
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            interaction.timestamp.time_since_epoch()).count();
        interactionJson["timestamp"] = timestamp;
        
        cassetteJson["interactions"].push_back(interactionJson);
    }
    
    std::string filePath = m_cassetteDir + "/" + m_currentCassette + ".json";
    std::ofstream file(filePath);
    file << cassetteJson.dump(2);
}

void HttpRecorder::loadCassette() {
    std::string filePath = m_cassetteDir + "/" + m_currentCassette + ".json";
    
    if (!fs::exists(filePath)) {
        throw std::runtime_error("Cassette not found: " + filePath);
    }
    
    std::ifstream file(filePath);
    json cassetteJson;
    file >> cassetteJson;
    
    m_interactions.clear();
    
    for (const auto& interactionJson : cassetteJson["interactions"]) {
        RecordedInteraction interaction;
        
        // Request reconstruction
        Request request;
        request.setMethod(static_cast<HttpMethod>(interactionJson["request"]["method"]));
        request.setUrl(interactionJson["request"]["url"]);
        request.setBody(interactionJson["request"]["body"]);
        
        for (const auto& [key, value] : interactionJson["request"]["headers"].items()) {
            request.addHeader(key, value);
        }
        
        interaction.request = request;
        
        // Response reconstruction
        HttpResult response;
        response.statusCode = interactionJson["response"]["status_code"];
        response.body = interactionJson["response"]["body"];
        response.headers = interactionJson["response"]["headers"];
        
        interaction.response = response;
        
        m_interactions.push_back(interaction);
    }
}

std::string HttpRecorder::generateRequestKey(const Request& request) {
    return std::to_string(static_cast<int>(request.getMethod())) + 
           ":" + request.getUrl() + 
           ":" + request.getBody();
}

} // namespace canaspad
```

## Phase 4: 自動化スクリプト

### 4.1 ローカル開発用スクリプト

```bash
#!/bin/bash
# scripts/run-all-tests.sh

set -e

echo "🚀 Starting test suite..."

# テスト環境の起動
echo "📦 Starting mock services..."
cd docker
docker-compose up -d
sleep 5

# サービスの健全性チェック
echo "🔍 Checking service health..."
curl -f http://localhost:1080/mockserver/status || {
    echo "❌ MockServer not ready"
    exit 1
}

curl -f http://localhost:8080/__admin/ || {
    echo "❌ WireMock not ready"
    exit 1
}

cd ..

# ユニットテストの実行
echo "🧪 Running unit tests..."
pio test -e native

# 統合テストの実行
echo "🔗 Running integration tests..."
pio test -e native-integration

# E2Eテストの実行（記録モード）
if [ "$1" = "--record" ]; then
    echo "📹 Running E2E tests in recording mode..."
    RECORD_MODE=1 pio test -e native --filter "e2e/*"
else
    echo "▶️ Running E2E tests in playback mode..."
    pio test -e native --filter "e2e/*"
fi

# クリーンアップ
echo "🧹 Cleaning up..."
cd docker
docker-compose down

echo "✅ All tests completed successfully!"
```

### 4.2 CI/CD用スクリプト

```bash
#!/bin/bash
# scripts/test-ci.sh

set -e

echo "🏗️ Running CI test suite..."

# 並列テスト実行
echo "🔄 Running tests in parallel..."

# バックグラウンドでMockサービス起動
cd docker
docker-compose up -d &
DOCKER_PID=$!
cd ..

# ユニットテストは並列実行
echo "🧪 Starting unit tests..."
pio test -e native &
UNIT_PID=$!

# Mockサービスの起動を待つ
sleep 10

# 統合テストを実行
echo "🔗 Starting integration tests..."
pio test -e native-integration &
INTEGRATION_PID=$!

# ユニットテストの完了を待つ
wait $UNIT_PID
echo "✅ Unit tests completed"

# 統合テストの完了を待つ
wait $INTEGRATION_PID
echo "✅ Integration tests completed"

# E2Eテスト（再生モード）
echo "▶️ Running E2E tests..."
pio test -e native --filter "e2e/*"
echo "✅ E2E tests completed"

# クリーンアップ
cd docker
docker-compose down
wait $DOCKER_PID

echo "🎉 CI test suite completed successfully!"
```

## 実行方法

### ローカル開発

```bash
# セットアップ
./scripts/setup-test-structure.sh

# テスト実行
./scripts/run-all-tests.sh

# 新しいセッションを記録
RECORD_MODE=1 ./scripts/run-all-tests.sh -l 3
```

### CI/CD

```bash
# CI環境での実行
./scripts/test-ci.sh
```

### 個別テスト

```bash
# ユニットテストのみ
pio test -e native

# 統合テストのみ
pio test -e native-integration

# 特定のテストファイル
pio test -e native --filter "test/unit/auth/*"
```

## 次のステップ

実装ガイドが完成しました。どのフェーズから実装を開始しますか？

1. **Phase 1**: Native環境の構築から開始
2. **Phase 2**: MockServer統合テスト
3. **Phase 3**: Recording Proxy実装
4. **全フェーズ**: 段階的に全て実装

選択してください。実装を進めます。