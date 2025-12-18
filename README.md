## 🍔 HttpClient_ESP32_Lib 🚀

開発体験を最優先に設計したESP32向けのHTTPクライアントライブラリです  ☕

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

### ✨ 特徴

* 💡 シンプルなAPIでHTTPリクエストを送信
* 🔒 SSL/TLS対応 (検証あり/なし)
* 🍪 クッキーの自動処理
* ➡️ リダイレクトの自動追跡
* 🔁 ネットワークエラー時の自動リトライ
* 🔌 プロキシ対応
* 🔒 ベーシック認証とBearer認証対応
* 📡 ストリーミング送信 (近日公開予定)
* ⏱️ タイムアウト設定
* 📊 進捗状況コールバック
* 📦 multipart/form-dataの送信

### 🛠️ インストール

1. Arduino IDEのライブラリマネージャで"HttpClient_ESP32_Lib"を検索してインストールします。
2. または、このリポジトリをダウンロードして、`HttpClient_ESP32_Lib`フォルダをArduino IDEのlibrariesフォルダにコピーします。

### 🚀 使い方

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "HttpClient.h"
#include "ConfigExample.h" // WiFiやAPIの接続情報

void setup() {
  Serial.begin(115200);

  // WiFi接続
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // NTPサーバ接続
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_host);
  while (!time(nullptr)) {
    delay(1000);
  }

  // HttpClientの設定
  canaspad::ClientOptions options;
  options.followRedirects = true;
  options.verifySsl = true;
  options.rootCA = isrg_root_x1; // 必要な場合はルートCA証明書を設定

  // HttpClientのインスタンスを作成
  canaspad::HttpClient client(options);

  // リクエストの作成
  canaspad::Request request;
  request.setUrl(api_url)
         .setMethod(canaspad::HttpMethod::GET); // GET, POST, PUT, DELETEなどを設定

  // リクエストの送信
  auto result = client.send(request);

  // 結果の処理
  if (result.isSuccess()) {
    const auto &httpResult = result.value();
    Serial.printf("Status code: %d\n", httpResult.statusCode);
    Serial.printf("Response body: %s\n", httpResult.body.c_str());
  } else {
    const auto &error = result.error();
    Serial.printf("HTTP Error: %s (Error code: %d)\n", error.message.c_str(), static_cast<int>(error.code));
  }
}

void loop() {
  // ループ処理
}
```

### 🔒 認証

ベーシック認証とBearer認証に対応しています。`ClientOptions`で認証タイプと認証情報を設定します。

```cpp
canaspad::ClientOptions options;
options.authType = canaspad::AuthType::Basic;
options.username = "user@example.com";
options.password = "password";

// または Bearer認証
options.authType = canaspad::AuthType::Bearer;
options.bearerToken = "your_bearer_token";
```

### 🍪 クッキー

クッキーは自動的に処理され、後続のリクエストに自動的に追加されます。クッキーの有効期限も考慮されます。

### ➡️ リダイレクト

リダイレクトは自動的に追跡されます。`ClientOptions`で`followRedirects`を`false`に設定すると、リダイレクトの追跡を無効にできます。

### 🔁 リトライ

ネットワークエラーが発生した場合、リクエストは自動的にリトライされます。`ClientOptions`で`maxRetries`と`retryDelay`を設定して、リトライの回数と遅延時間を変更できます。

### 🔌 プロキシ

プロキシを使用する場合は、`ClientOptions`で`proxyUrl`を設定します。プロキシ認証が必要な場合は、URLにユーザ名とパスワードを含めます。

```cpp
options.proxyUrl = "http://proxy.example.com:8080";

// プロキシ認証が必要な場合
options.proxyUrl = "http://user:password@proxy.example.com:8080";
```

### 📦 multipart/form-data

`multipart/form-data`を送信するには、`Request`の`setMultipartFormData()`メソッドを使用します。

```cpp
std::vector<std::pair<std::string, std::string>> formData = {
  {"name", "John Doe"},
  {"age", "30"},
  {"file", "file_content"}
};
request.setMultipartFormData(formData);
```

### ⏱️ タイムアウト

タイムアウトは、接続、読み込み、書き込み操作ごとに設定できます。`HttpClient`の`setTimeouts()`メソッド、または個別のメソッドを使用してタイムアウトを設定します。

```cpp
canaspad::HttpClient::Timeouts timeouts;
timeouts.connect = std::chrono::seconds(10);
timeouts.read = std::chrono::seconds(30);
timeouts.write = std::chrono::seconds(10);
client.setTimeouts(timeouts);

// または 個別設定
client.setConnectionTimeout(std::chrono::seconds(10));
```

### 📊 進捗状況コールバック

進捗状況コールバックを設定すると、リクエストの進捗状況を取得できます。コールバック関数は、読み込まれたバイト数とコンテンツ長を受け取ります。

```cpp
client.setProgressCallback([](size_t bytesRead, size_t contentLength) {
  Serial.printf("Progress: %zu / %zu bytes\n", bytesRead, contentLength);
});
```

### 📝 ライセンス

このライブラリはGPL3ライセンスで提供されています。

## 📚 ドキュメント

### 🚀 開発環境セットアップ

ESP32実機でのテスト環境構築から実際の開発まで、完全に検証済みの手順を提供します：

- **[WSL ESP32完全構築ガイド](docs/WSL_ESP32_COMPLETE_GUIDE.md)** 🔧  
  WSL環境でのESP32開発環境の完全セットアップ手順（usbipd-winからテスト実行まで）

- **[ESP32開発用スクリプト](scripts/README.md)** ⚡  
  ESP32の自動接続・テスト実行を効率化するスクリプト集

### 🧪 テスト & デバッグ

実際のESP32実機テストで検証済みの手順とトラブルシューティング：

- **[実機テスト手順書](docs/TESTING_PROCEDURE.md)** 📋  
  HttpClientライブラリの実機テスト詳細とAPIの実際の使用例

- **[トラブルシューティングガイド](docs/TROUBLESHOOTING_GUIDE.md)** 🚨  
  実際に遭遇した問題と検証済みの解決策（環境構築からコード実行まで）

### 💻 日常的な開発

効率的な開発ワークフローとベストプラクティス：

- **[開発ワークフロー](docs/DEVELOPMENT_WORKFLOW.md)** 🔄  
  日常的な開発フローの最適化とCI/CD連携

### 🛠️ 追加リソース

- [🧪 ツールとテスト実行ガイド](docs/TOOLS_AND_TESTING.md) - テスト実行方法、開発ツール
- [📋 実装ガイド](docs/implementation_guide.md) - 新機能追加時の参考  
- [🗺️ 移行ロードマップ](docs/migration_roadmap.md) - 将来の計画

## 🎯 クイックスタート

### ESP32実機テストを今すぐ始める

1. **環境構築**: [WSL ESP32完全構築ガイド](docs/WSL_ESP32_COMPLETE_GUIDE.md) に従ってセットアップ
2. **デバイス接続**: `./scripts/esp32_attach.sh` でESP32を自動接続
3. **テスト実行**: `~/.platformio/penv/bin/pio test -e m5stack-atom --upload-port /dev/ttyUSB0`

### 問題が発生した場合

- **セットアップ問題**: [WSL ESP32完全構築ガイド](docs/WSL_ESP32_COMPLETE_GUIDE.md) の手順を確認
- **接続問題**: [トラブルシューティングガイド](docs/TROUBLESHOOTING_GUIDE.md) で解決策を検索
- **テスト問題**: [実機テスト手順書](docs/TESTING_PROCEDURE.md) で正しいAPIの使用方法を確認
