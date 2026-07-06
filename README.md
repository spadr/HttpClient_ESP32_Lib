# 🍔 HttpClient_ESP32_Lib

**ESP32開発者のための、モダンで堅牢な C++ HTTP クライアントライブラリ** 🚀

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

標準の `HTTPClient` よりもリッチな設計を採用しています。HTTPS、認証、クッキー管理、リトライ処理などを利用できます。

## ✨ 特徴

*   💡 **モダンな API**: メソッドチェーンで直感的にリクエストを構築
*   🔒 **HTTPS 完全対応**: SSL/TLS 検証、ルート CA 設定、時刻同期ヘルパー
*   🔁 **堅牢な通信**: リダイレクト自動追跡、自動リトライ、タイムアウト管理
*   🍪 **状態管理**: Cookie の自動維持（`enableCookies(true)` で有効化）
*   🔑 **認証**: Basic 認証、Bearer (Token) 認証をネイティブサポート

## 🚀 クイックスタート

### インストール

**PlatformIO** (`platformio.ini`):
```ini
lib_deps = 
    https://github.com/canaspad/HttpClient_ESP32_Lib.git
```

**Arduino IDE**:
ZIPとしてダウンロードし、「スケッチ」→「ライブラリをインクルード」→「.ZIP形式のライブラリをインストール」から追加してください。

サンプルスケッチは [`examples/BasicRequest`](examples/BasicRequest) を参照してください（Arduino IDE: `BasicRequest.ino` / PlatformIO: `src/main.cpp`）。

### 基本的な使い方

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <HttpClient.h>

using namespace canaspad;

void setup() {
    Serial.begin(115200);
    
    // 1. WiFi 接続
    WiFi.begin("SSID", "PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    // 2. 時刻同期 (HTTPS通信に必須)
    HttpClient::syncTime(); 

    // 3. クライアントの設定
    ClientOptions options;
    options.followRedirects = true;
    options.verifySsl = true; // 本番環境では true 推奨
    HttpClient client(options);
    client.enableCookies(true); // Cookie 自動送信を有効化

    // 4. リクエストの構築 (Fluent Interface)
    Request request;
    request.setUrl("https://httpbin.org/post")
           .setMethod(HttpMethod::POST)
           .addHeader("Content-Type", "application/json")
           .setBody("{\"message\": \"Hello ESP32!\"}");

    // 5. 送信と結果処理
    auto result = client.send(request);

    if (result.isSuccess()) {
        Serial.printf("Status: %d\n", result.value().statusCode);
        Serial.println(result.value().body.c_str());
    } else {
        Serial.printf("Error: %s (Code: %d)\n", 
            result.error().message.c_str(), (int)result.error().code);
    }
}

void loop() {}
```

## 💡 便利な機能

### 認証 (Basic / Bearer)
```cpp
ClientOptions options;
// Basic 認証
options.authType = AuthType::Basic;
options.username = "user";
options.password = "pass";

// または Bearer (Token) 認証
options.authType = AuthType::Bearer;
options.bearerToken = "your-access-token";
```

### タイムアウト設定
```cpp
HttpClient::Timeouts timeouts;
timeouts.connect = std::chrono::seconds(10); // 接続タイムアウト
timeouts.read    = std::chrono::seconds(30); // 読み込みタイムアウト
timeouts.write   = std::chrono::seconds(30); // 書き込みタイムアウト
client.setTimeouts(timeouts);
```

### Multipart/form-data 送信
```cpp
// テキストフィールド
request.setMultipartFormData({
    {"username", "john_doe"}
});

// ファイルパート (filename / Content-Type 付き)
request.addMultipartFile(
    "file",
    "report.txt",
    "file content here",
    "text/plain"
);
```

### プロキシ
```cpp
ClientOptions options;
options.proxyUrl = "http://user:pass@proxy.example.com:3128";
HttpClient client(options);
```

### ストリーミング受信
```cpp
auto result = client.sendStreaming(request, [](const char *data, size_t size) {
    // レスポンス本文を逐次処理 (result.value().body は空)
});
```

### リクエストキャンセル
```cpp
client.cancel("request-id");
```

### 進捗コールバック
```cpp
client.setProgressCallback([](size_t received, size_t total) {
    Serial.printf("Progress: %zu / %zu\n", received, total);
});
client.setResponseBodyCallback([](const char *data, size_t size) {
    // 本文チャンクを逐次受信
});
```

## 📚 ドキュメント

より詳細な情報はこちらを参照してください。

*   **[トラブルシューティング](docs/TROUBLESHOOTING_GUIDE.md)**: うまく動かない場合
*   **[開発環境構築 (WSL)](docs/WSL_ESP32_COMPLETE_GUIDE.md)**: WSL での ESP32 開発環境セットアップ
*   **[開発ワークフロー](docs/DEVELOPMENT_WORKFLOW.md)**: テスト実行と実機開発の手順

## 📝 ライセンス

GPL v3
