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

### 🛠️ 検証用環境
[ドキュメント](docs/verification_server.md) を参照して、検証用サーバをセットアップしてください。
