# ESP32実機テスト実行ガイド

## 🎯 概要

HttpClient ESP32ライブラリのESP32実機でのテスト実行手順とトラブルシューティングガイドです。

## ✅ テスト実装状況

### 実装済みテスト（test/main.cpp）

1. **test_basic_http_connection()**
   - HTTP GET リクエスト（httpbin.org）
   - WiFi接続確認
   - レスポンス200確認

2. **test_https_connection()**
   - HTTPS GET リクエスト（httpbin.org）
   - SSL証明書検証（Let's Encrypt ISRG Root X1）
   - セキュア通信確認

3. **test_http_post()**
   - HTTP POST リクエスト（JSON）
   - Content-Type ヘッダー設定
   - リクエストボディ送信

### 設定ファイル
- **src/Config.h** - WiFi認証情報とSSL証明書
- **test/Config.h** - テスト用設定（予備）

## 🚀 実行手順

### 1. 環境準備

```bash
# プロジェクトルートで実行
cd HttpClient_ESP32_Lib

# ESP32準備（接続確認）
pio device list
```

### 2. WiFi設定確認

**src/Config.h** を編集:
```cpp
namespace Config {
    const char *ssid = "あなたのWiFi_SSID";
    const char *password = "あなたのWiFiパスワード";
    // ...
}
```

### 3. ESP32デバイス接続

- ESP32をUSBケーブルでPCに接続
- デバイスが認識されているか確認:

```bash
# Linux/WSL
ls /dev/ttyUSB* /dev/ttyACM*

# Windows
# デバイスマネージャーでCOMポート確認

# PlatformIO デバイス確認
pio device list
```

### 4. テスト実行

```bash
# ESP32実機テスト実行
pio test -e m5stack-atom

# 詳細ログ付き実行
pio test -e m5stack-atom -v

# シリアルモニター（テスト後）
pio device monitor -e m5stack-atom
```

## 📋 テスト妥当性確認

**実機なしでテストコードを検証:**

```bash
# テストロジック検証実行
g++ -std=c++17 test/test_validation/validate_esp32_tests.cpp -o validate_esp32_tests
./validate_esp32_tests
```

**期待される出力:**
```
=== ESP32 Test Code Validation ===
=== Running E2E Real Server Tests ===
Running: test_basic_http_connection... PASSED
Running: test_https_connection... PASSED  
Running: test_http_post... PASSED

✓ All test implementations are valid!
Ready for ESP32 hardware testing.
```

## 🔧 トラブルシューティング

### PlatformIO プラットフォーム問題

**症状:** `Error: Detected unknown package 'espressif32'`

**解決策:**
```bash
# PlatformIO更新
pip install -U platformio

# プラットフォーム強制インストール
pio platform install espressif32

# 設定確認
pio platform list
```

### ESP32デバイス認識問題

**Linux/WSL:**
```bash
# ユーザー権限追加
sudo usermod -a -G dialout $USER

# USB権限確認
ls -la /dev/ttyUSB* /dev/ttyACM*
```

**Windows:**
- CP2102/CH340ドライバーインストール
- デバイスマネージャーでCOMポート確認

### WiFi接続問題

**test/main.cpp の setUp() 関数で確認:**
```cpp
void setUp(void) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to WiFi...");
        // 20回リトライ（10秒）
    }
}
```

**対処法:**
1. SSID/パスワード再確認
2. 2.4GHzネットワーク使用確認
3. WiFi信号強度確認

### HTTPS証明書問題

**証明書期限確認:**
- ISRG Root X1: 2035年まで有効
- 必要に応じて証明書更新

**デバッグ:**
```cpp
// HTTPから開始してHTTPSに移行
Request request("http://httpbin.org/get");  // HTTP
Request request("https://httpbin.org/get"); // HTTPS
```

## 📊 期待されるテスト結果

### 成功時のシリアル出力例

```
Starting WiFi connection...
..........
WiFi connected
IP address: 192.168.1.100

Synchronizing time with NTP server...
Time synchronized:
Monday, July 22 2024 15:30:45

=== Running E2E Real Server Tests ===
Testing basic HTTP connection...
✓ HTTP GET successful: 200

Testing HTTPS connection...  
✓ HTTPS GET successful: 200

Testing HTTP POST...
✓ HTTP POST successful: 200

3 Tests 0 Failures 0 Ignored
```

### PlatformIO テスト出力例

```
Processing * in m5stack-atom environment
Building & Uploading...
Testing...
Test    Environment    Status    Duration
------  -------------  --------  ----------
*       m5stack-atom   PASSED    00:00:15.234
========================= 1 succeeded in 00:00:15.234 =========================
```

## 🎯 パフォーマンス指標

- **WiFi接続**: 通常5-10秒
- **HTTP GET**: 応答時間 < 3秒
- **HTTPS GET**: 応答時間 < 5秒（SSL握手含む）
- **HTTP POST**: 応答時間 < 3秒

## 🔄 継続的改善

### 追加できるテスト
1. **エラーハンドリング**: 無効なURL、ネットワーク切断
2. **レスポンス検証**: JSON解析、ヘッダー確認
3. **パフォーマンス**: 応答時間測定、メモリ使用量
4. **認証**: Basic認証、Bearer token
5. **プロキシ**: HTTP proxy経由通信

### 監視とログ
```cpp
// レスポンス詳細出力
if (result.isOk()) {
    auto response = result.unwrap();
    Serial.printf("Status: %d\n", response.getStatusCode());
    Serial.printf("Body: %s\n", response.getBody().c_str());
    Serial.printf("Headers: %s\n", response.getHeaders().c_str());
}
```

## 📁 関連ファイル

- `test/main.cpp` - ESP32テストメイン
- `src/Config.h` - WiFi・SSL設定
- `test/test_validation/validate_esp32_tests.cpp` - テスト妥当性確認
- `docs/TOOLS_AND_TESTING.md` - 全体テスト戦略

**ESP32実機テストの準備が完了しています！** 🎉