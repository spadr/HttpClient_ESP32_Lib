# HttpClient ESP32 完全テスト分離ガイド

## 🎯 概要

このガイドでは、**「PC上のUnit/Integration」「ESP32実機のE2E」**を完全に分離し、「全部まとめて実行」も可能な構成について詳しく説明します。

## 📁 1️⃣ フォルダ構成

```
HttpClient_ESP32_Lib/
├── src/
│   ├── esp32/                  ← ESP32専用ソース
│   │   └── main.cpp           ← ESP32実機用メイン（本番・テスト両用）
│   ├── native/                ← PC テスト用スタブ  
│   │   └── main.cpp           ← Native環境用ダミーmain
│   ├── HttpClient.h           ← ライブラリ本体
│   ├── core/                  ← コア機能
│   ├── utils/                 ← ユーティリティ
│   └── auth/                  ← 認証機能
└── test/
    ├── unit/                  ← ユニットテスト
    │   └── test_*.cpp        
    ├── integration/           ← モックサーバ等を使う結合テスト
    │   └── test_*.cpp
    ├── e2e/                   ← 実機相手テスト
    │   └── test_*.cpp
    ├── test_unit_utils.cpp    ← PC用ユニットテスト（ルート配置）
    └── test_e2e_esp32.cpp     ← ESP32用E2Eテスト（ルート配置）
```

**テスト階層のポイント:**
- PlatformIOが推奨する「`test/<suite>/...`」ルールに準拠
- `test_filter`には**ディレクトリ名**または**ファイル名**を指定
- PC用とESP32用のテストファイルをルートに分離配置

## 🔧 2️⃣ `platformio.ini` の最適化された構成

```ini
[platformio]
default_envs = test_unit        ; 'pio test' だけでユニット実行

; ===== COMMON CONFIGURATIONS =====

; ---------- PC Native Common ----------
[common_native]
platform = native
build_flags = -std=gnu++17
              -DNATIVE_TEST
              -DARDUINO_ARCH_NATIVE
              -Itest/helpers
              -Itest/fixtures
build_src_filter = +<native/*> 
                   +<utils/*> +<auth/*> +<cookie/*> 
                   +<core/Request.*> +<core/Response.*> +<core/HttpResult.*> +<core/CommonTypes.*>
                   -<esp32/*> -<core/WiFiSecureConnection.*> -<core/Connection.*> 
                   -<core/ConnectionPool.*> -<core/mock/*>
test_build_src = yes            ; Link src/ with tests
lib_deps = throwtheswitch/Unity@^2.5.2

; ---------- ESP32 Common ----------  
[common_esp32]
platform = espressif32
board = m5stack-atom
framework = arduino
upload_speed = 1500000
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
build_unflags = -std=gnu++11
build_flags = -std=gnu++17
              -DCORE_DEBUG_LEVEL=5
              -DCONFIG_H_EXISTS
build_src_filter = +<*> -<native/*> -<esp32/main.cpp>
test_build_src = yes            ; Link src/ with tests for ESP32
lib_deps = 
    bblanchon/ArduinoJson@^6.19.4
    throwtheswitch/Unity@^2.5.2

; ======= TEST ENVIRONMENTS =======

; 1. Unit Tests (PC: Fastest)
[env:test_unit]
extends = common_native
test_filter = test_unit_utils.cpp
test_ignore = test_e2e_esp32.cpp
build_flags = ${common_native.build_flags}
              -DUNIT_TEST

; 2. Integration Tests (PC + MockServer)
[env:test_integration]
extends = common_native
test_filter = integration/*
build_flags = ${common_native.build_flags}
              -DINTEGRATION_TEST

; 3. All PC Tests (Unit + Integration together)
[env:test_all]
extends = common_native
test_filter = unit/*|integration/*
build_flags = ${common_native.build_flags}
              -DALL_TESTS

; 4. E2E Tests (ESP32 Real Hardware)
[env:test_esp32]
extends = common_esp32
test_filter = test_e2e_esp32.cpp
build_flags = ${common_esp32.build_flags}
              -DE2E_TEST

; ======= PRODUCTION ENVIRONMENTS =======

; ESP32 Production Build
[env:m5stack-atom]
extends = common_esp32
build_flags = ${common_esp32.build_flags}
              -DPRODUCTION

; ======= LEGACY ENVIRONMENTS (backward compatibility) =======

[env:native]
extends = env:test_unit

[env:native_integration]
extends = env:test_integration
```

### 🔑 重要なポイント

| 設定項目 | 役割 | 使用環境 |
|----------|------|----------|
| `build_src_filter` | src/にある**実機用/PC用のmain.cpp**を切替 | 全環境 |
| `test_build_src = yes` | *テスト時にもsrc/をリンク*したいときに必須 | 全テスト環境 |
| `test_filter` | ディレクトリ単位またはファイル単位でテストスイートを選別 | 全テスト環境 |
| `test_ignore` | 特定のテストファイルを除外 | Native環境 |
| `default_envs` | `pio test`だけで一番軽いユニットが走るよう設定 | グローバル |

## 🚀 3️⃣ 実行パターン

### 基本的な実行コマンド

| コマンド | 実行内容 | 実行時間 | 実行環境 |
|----------|----------|----------|----------|
| `pio test` | `test_unit`（ユニットのみ） | 3-10秒 | PC Native |
| `pio test -e test_unit` | ユニットテスト明示実行 | 3-10秒 | PC Native |
| `pio test -e test_integration` | Integration だけ | 10-30秒 | PC Native + Mock |
| `pio test -e test_all` | Unit + Integration（PC） | 15-40秒 | PC Native |
| `pio test -e test_esp32 --upload-port /dev/ttyUSB0` | 実機E2E（ビルド→書込み→実行） | 1-3分 | ESP32実機 |

### 高度な実行オプション

```bash
# 詳細ログ付き統合テスト
pio test -e test_integration -v

# ESP32実機テスト（書き込みスキップ）
pio test -e test_esp32 --without-uploading

# ESP32実機テスト（ビルドのみ）
pio test -e test_esp32 --without-uploading --without-testing

# テスト一覧表示のみ
pio test --list-tests

# 全環境でのビルド確認
pio run
```

## 🔧 4️⃣ テストコード側の切り替え例

### PC Native用ユニットテスト (test/test_unit_utils.cpp)

```cpp
#ifdef NATIVE_TEST
#include "helpers/TestEnvironment.h"
#define ARDUINO_ARCH_NATIVE
#endif

#include "helpers/simple_unity.h"
#include "../src/utils/Utils.h"
#include "../src/utils/HttpMethod.h"
#include "../src/core/HttpResult.h"
#include "helpers/AssertHelpers.h"

using namespace canaspad;

void setUp(void) {
    // テスト前の初期化
}

void tearDown(void) {
    // テスト後のクリーンアップ
}

void test_extract_scheme_http() {
    std::string url = "http://example.com/path";
    std::string scheme = Utils::extractScheme(url);
    TEST_ASSERT_EQUAL_STRING("http", scheme.c_str());
}

// ... その他のテスト関数

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_extract_scheme_http);
    // ... その他のテスト実行
    return UNITY_END();
}
```

### ESP32実機用E2Eテスト (test/test_e2e_esp32.cpp)

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <unity.h>
#include "Config.h"
#include "../src/HttpClient.h"
#include "../src/core/Request.h"

using namespace canaspad;

void setUp(void) {
    // 各テスト前の初期化
}

void tearDown(void) {
    // 各テスト後のクリーンアップ
}

void setup() {
    delay(2000); // シリアルモニタ接続待ち
    Serial.begin(115200);
    Serial.println("=== HttpClient ESP32 Library - E2E Tests ===");
    
    // WiFi接続
    WiFi.begin(Config::ssid, Config::password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    
    // NTP時刻同期
    configTime(Config::gmt_offset_sec, Config::daylight_offset_sec, Config::ntp_host);
    
    // Unity テストフレームワーク開始
    UNITY_BEGIN();
    RUN_TEST(test_basic_http_connection);
    RUN_TEST(test_https_connection);
    RUN_TEST(test_http_post);
    UNITY_END();
}

void loop() {
    delay(1000);
}

void test_basic_http_connection() {
    HttpClient client;
    Request request;
    request.setUrl("https://httpbin.org/get");
    request.setMethod(canaspad::HttpMethod::GET);
    
    auto result = client.send(request);
    TEST_ASSERT_TRUE(result.isSuccess());
    
    if (result.isSuccess()) {
        auto response = result.value();
        TEST_ASSERT_TRUE(response.statusCode == 200);
        Serial.printf("✓ HTTP GET successful: %d\n", response.statusCode);
    }
}

// ... その他のE2Eテスト関数
```

### Native用ダミーmain (src/native/main.cpp)

```cpp
#if defined(NATIVE_TEST) && !defined(UNIT_TEST) && !defined(INTEGRATION_TEST) && !defined(ALL_TESTS)

#include <iostream>

// Native環境用のダミーmain関数
// テスト実行時はUnityフレームワークがmain()を提供するため、
// このmain()は「pio run」でsrc/だけをビルドした場合のみ使用される
int main() {
    std::cout << "HttpClient ESP32 Library - Native Build Environment" << std::endl;
    std::cout << "This is a stub main() for native builds." << std::endl;
    std::cout << "For tests, use: pio test -e <environment>" << std::endl;
    return 0;
}

#endif // NATIVE_TEST && !TEST_DEFINES
```

## 🔧 5️⃣ 現在の動作状況

### ✅ 動作確認済み

1. **フォルダ分割によるソース管理**
   - ESP32とNative環境での適切なソースファイル分離
   - `build_src_filter`による精密な制御

2. **PC Native環境でのユニットテスト**
   ```bash
   # 33個のテストが成功（過去の実行結果）
   pio test -e test_unit
   ```

3. **ESP32実機でのE2Eテスト**
   ```bash
   # WiFi接続、HTTP/HTTPS通信テストが成功
   pio test -e test_esp32 --upload-port /dev/ttyUSB0
   ```

### 🔄 調整が必要な項目

1. **PlatformIOキャッシュシステム**
   - 環境切り替え時のビルドキャッシュの競合
   - `.pio`ディレクトリの削除で解決可能

2. **テストファイルの配置**
   - PlatformIOのテスト自動検出との整合性
   - ルートディレクトリ配置による動作確認

## 🚨 6️⃣ よくあるハマりどころと解決策

### 問題1: `Arduino.h: No such file or directory`

**症状:** Native環境でESP32用コードをビルド
```
error: Arduino.h: No such file or directory
```

**解決策:** `build_src_filter`で適切にフォルダ分離
```ini
# Native環境設定
build_src_filter = +<native/*> +<utils/*> -<esp32/*>  ✅
```

### 問題2: `multiple definition of 'setup'`

**症状:** ESP32環境でmain.cppとテストファイルが衝突
```
multiple definition of `setup'; test_esp32.cpp.o: first defined here
```

**解決策:** ESP32テスト環境でsrc/esp32/main.cppを除外
```ini
# ESP32テスト環境設定
build_src_filter = +<*> -<native/*> -<esp32/main.cpp>  ✅
```

### 問題3: ビルドキャッシュの競合

**症状:** 環境切り替え時にビルドエラー
```
*** [.pio/build/test_unit/.sconsign310.dblite] No such file or directory
```

**解決策:** ビルドキャッシュのクリア
```bash
pio run -t clean
# または
rm -rf .pio
```

### 問題4: テスト検出されない

**症状:** `Collected 1 tests (*)` but no specific tests
```
Error: Nothing to build. Please put your test suites to '/path/to/test' folder
```

**解決策:** ファイル配置とフィルタ設定の確認
```ini
test_filter = test_unit_utils.cpp        # 具体的なファイル名
test_ignore = test_e2e_esp32.cpp         # 除外ファイル名
```

## 💡 7️⃣ 開発効率化のヒント

### エイリアス設定

```bash
# ~/.bashrc に追加
alias pio='~/.platformio/penv/bin/pio'

# テスト実行エイリアス
alias test-unit='pio test -e test_unit'
alias test-integration='pio test -e test_integration'  
alias test-all='pio test -e test_all'
alias test-esp32='pio test -e test_esp32 --upload-port /dev/ttyUSB0'

# ESP32デバイス管理
alias esp32-attach='./scripts/esp32_attach.sh'
alias esp32-build='pio run -e m5stack-atom'

# ビルドクリーンアップ
alias clean-build='pio run -t clean'
alias clean-all='rm -rf .pio'
```

### 日常的な開発フロー

```bash
# 1. 朝の開発開始
esp32-attach              # ESP32デバイス接続確認
test-unit                 # 高速ユニットテスト（3-10秒）

# 2. 機能開発中
# コード変更後
test-unit                 # 基本ロジック確認（高速）

# 3. 機能完成後
test-integration          # 統合テスト（10-30秒）
test-esp32               # 実機E2Eテスト（1-3分）

# 4. 問題発生時
clean-all                # ビルドキャッシュ完全削除
test-unit                # 再テスト実行
```

## 📊 8️⃣ パフォーマンス比較

| テスト種別 | 実行時間 | 実行環境 | 信頼性 | 用途 |
|------------|----------|----------|--------|------|
| **Unit Tests** | 3-10秒 | PC Native | 🟡 基本ロジック | 開発中の素早い確認 |
| **Integration Tests** | 10-30秒 | PC + MockServer | 🟠 通信層 | プロトコル層の確認 |
| **All PC Tests** | 15-40秒 | PC + Mock/Native | 🟠 PC環境統合 | まとめて確認 |
| **ESP32 E2E Tests** | 1-3分 | ESP32実機 + 実ネット | 🟢 完全動作 | 最終リリース前確認 |

## 🎯 9️⃣ この構成の利点

### 開発者体験の向上

1. **段階的テスト実行**
   - 最速3秒からのフィードバックサイクル
   - 問題の早期発見・修正

2. **環境の完全分離**
   - PC環境とESP32環境の干渉なし
   - 各環境に最適化されたテスト

3. **柔軟な実行選択**
   - 開発段階に応じた適切なテスト選択
   - CI/CDでの多段階テスト対応

### 保守性・拡張性

1. **明確な責任分離**
   - ユニット：基本ロジック
   - 統合：通信・プロトコル層
   - E2E：完全な動作確認

2. **新しいプラットフォーム対応**
   - フォルダ分割による容易な拡張
   - `common_*`設定による設定共有

## 📚 参考リンク

- [PlatformIO Test Hierarchy](https://docs.platformio.org/en/stable/advanced/unit-testing/structure/hierarchy.html)
- [PlatformIO test_build_src](https://docs.platformio.org/en/latest/projectconf/sections/env/options/test/test_build_src.html)
- [PlatformIO test_filter](https://docs.platformio.org/en/stable/projectconf/sections/env/options/test/test_filter.html)
- [PlatformIO build_src_filter](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_src_filter.html)

---

**この構成により、PC上の高速テストから実機での完全テストまで、開発段階に応じた最適なテスト実行が可能になります。** 🚀