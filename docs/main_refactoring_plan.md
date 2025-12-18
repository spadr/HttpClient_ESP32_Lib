# HttpClient_ESP32_Lib - main.cpp リファクタリング計画

**作成日時**: 2025/07/24 18:04  
**対象**: src/main.cpp およびエントリーポイント関連ファイルの最適化

## 📋 現状問題分析

### 🔍 特定された問題
1. **メイン関数の重複構造**
   - `src/main.cpp` (現在のメイン、条件付きコンパイル使用)
   - `src/esp32/main.cpp` (ESP32実機テスト用)
   - `src/native/main.cpp` (PC環境用スタブ)

2. **コードの重複問題**
   - WiFi接続処理が`src/main.cpp:26-33`と`src/main.cpp:47-54`で重複
   - NTP同期処理が`src/main.cpp:35-44`と`src/main.cpp:56-65`で重複

3. **条件付きコンパイル地獄**
   ```cpp
   #ifndef PIO_UNIT_TESTING
   #ifndef ARDUINO_ARCH_NATIVE
   #include <Arduino.h>
   #include <WiFi.h>
   #else
   #include "native_arduino_compat.h"
   #endif
   ```

4. **モック/実機コードの混在**
   - 同一ファイル内でモック設定(84行目)と実機テスト処理が混在
   - テスト目的が不明確

## 🎯 選択したベストプラクティス

### 方法1: 共通ロジック関数化 + 入口分離（最推奨）

**典型的な解決パターン3つのうち、方法1を選択**

1. ✅ **共通ロジックを関数化して「入口だけ」切り替える**（採用）
2. ⚪ ハード依存部を抽象化して差し替える（HALパターン）（部分採用）
3. ❌ 条件付きインクルード＋スタブ（非推奨、現在の問題の原因）

### 🏗️ 新アーキテクチャ設計

```
src/
├── app/                    # アプリケーションレイヤー
│   ├── App.hpp            # 共通アプリケーションインターフェース
│   ├── App.cpp            # 共通ロジック実装
│   └── HttpClientDemo.cpp # デモ/テスト実装
├── hal/                   # ハードウェア抽象化レイヤー
│   ├── HalInterface.hpp   # HAL抽象インターフェース
│   ├── esp32/            # ESP32実装
│   │   └── EspHal.cpp
│   └── native/           # PC実装  
│       └── NativeHal.cpp
├── esp32_main.cpp        # ESP32エントリーポイント（新規）
├── native_main.cpp       # PCエントリーポイント（既存改修）
└── main.cpp              # 削除予定
```

## 📝 実装計画詳細

### Phase 1: 共通ロジック抽出

**1. App.hpp インターフェース設計**
```cpp
namespace app {
    void init();           // 初期化処理（WiFi, NTP等）
    void runDemo();        // HttpClientデモ実行
    void tick();           // ループ処理（必要時）
}
```

**2. 重複排除対象**
- WiFi接続処理の一元化
- NTP同期処理の一元化
- HttpClientデモロジックの共通化

### Phase 2: エントリーポイント分離

**1. esp32_main.cpp（新規作成）**
```cpp
#if defined(ARDUINO)
#include <Arduino.h>
#include "app/App.hpp"

void setup() { 
    app::init(); 
    app::runDemo();
}

void loop() {  
    app::tick(); 
}
#endif
```

**2. native_main.cpp（改修）**
```cpp
#ifndef ARDUINO
#include "app/App.hpp"
#include <chrono>
#include <thread>

int main() {
    app::init();
    app::runDemo();
    
    while(true) {
        app::tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
#endif
```

### Phase 3: HAL抽象化（段階的実装）

**1. 抽象化対象**
- WiFi接続管理
- NTP時刻同期
- シリアル出力
- タイマー/遅延処理

**2. インターフェース例**
```cpp
class HalInterface {
public:
    virtual bool connectWiFi(const char* ssid, const char* password) = 0;
    virtual bool syncNTP(const char* server, long gmtOffset, int daylightOffset) = 0;
    virtual void printSerial(const std::string& message) = 0;
    virtual void delay(uint32_t ms) = 0;
};
```

### Phase 4: platformio.ini最適化

**build_src_filter設定**
```ini
[env:m5stack-atom]  # ESP32環境
extends = common_esp32
build_src_filter = +<*> 
                   +<hal/esp32/*>
                   -<native_main.cpp> 
                   -<hal/native/*>
                   -<main.cpp>

[env:test_unit]     # PC環境
extends = common_native
build_src_filter = +<*>
                   +<hal/native/*>
                   -<esp32_main.cpp>
                   -<hal/esp32/*>
                   -<main.cpp>
```

## ✅ 期待効果

1. **重複排除**: WiFi/NTP処理コードの一元化
2. **テスト容易性**: PC環境でのロジックテスト向上  
3. **保守性**: 条件付きコンパイル削減、可読性向上
4. **拡張性**: 新プラットフォーム追加の容易性
5. **安定性**: main.cpp物理切り替えより安定した動作

## 🚀 実装手順

1. **Phase 1**: App.hpp/App.cpp作成、共通ロジック移行
2. **Phase 2**: esp32_main.cpp作成、native_main.cpp改修
3. **Phase 3**: HAL抽象化実装（段階的）
4. **Phase 4**: platformio.ini調整、テスト実行
5. **Phase 5**: 旧main.cpp削除、ドキュメント更新

## 📚 参考資料

- PlatformIO build_src_filter ドキュメント
- Arduino フレームワーク setup()/loop() 仕様
- C++ HALパターン実装例

---
**Next Steps**: Phase 1から順次実装開始