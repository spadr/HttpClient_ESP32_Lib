# HttpClient ESP32 テスト実行完全ガイド

## 🎯 概要

このガイドでは、HttpClient ESP32ライブラリの各種テスト実行方法を、**フォルダ分割方式による実装**で詳しく説明します。

## 📁 フォルダ分割によるプロジェクト構造

```
HttpClient_ESP32_Lib/
├── src/
│   ├── esp32/main.cpp          # ESP32実機用メイン（Arduino依存）
│   ├── native/main.cpp         # Native環境用メイン（Arduino依存なし）
│   ├── HttpClient.h            # ライブラリ本体
│   ├── core/                   # コア機能
│   ├── utils/                  # ユーティリティ
│   └── auth/                   # 認証機能
├── test/
│   ├── unit/                   # ユニットテスト（高速）
│   ├── integration/            # 統合テスト（MockServer）
│   └── e2e/                    # End-to-Endテスト（実機）
└── platformio.ini              # テスト環境設定
```

## 🔧 フォルダ分割のメリット

### 方法2: フォルダ分割方式の採用理由

| 利点 | 詳細 |
|------|------|
| **明確な分離** | ESP32用とNative用のコードが完全に分離 |
| **保守性** | 各環境専用のコードで衝突なし |
| **可読性** | `build_src_filter`で明示的に制御 |
| **拡張性** | 新しいプラットフォーム追加が容易 |

### platformio.ini での設定

```ini
# ESP32環境
[env:m5stack-atom]
build_src_filter = +<*> +<esp32/*> -<native/*>

# Native環境  
[env:test_unit]
build_src_filter = +<*> +<native/*> -<esp32/*>
```

## 🚀 テスト実行方法

### 方法1: 推奨実行順序

#### Step 1: ユニットテスト（最優先・最高速）

```bash
# ネイティブ環境でのユニットテスト
~/.platformio/penv/bin/pio test -e test_unit

# エイリアス使用
test-unit
```

**使用されるファイル:**
- `src/native/main.cpp` - Native用エントリーポイント
- `test/unit/` 以下のテストファイル

#### Step 2: ESP32実機テスト（現在動作確認済み）

```bash
# ESP32接続確認
./scripts/esp32_attach.sh

# 実機テスト実行（動作確認済み）
~/.platformio/penv/bin/pio test -e m5stack-atom --upload-port /dev/ttyUSB0
```

**使用されるファイル:**
- `src/esp32/main.cpp` - ESP32用エントリーポイント
- `test/e2e/` 以下のテストファイル

### 方法2: 個別テスト実行（回避策）

PlatformIOのテスト検出に問題がある場合の直接実行:

```bash
# Utils テスト例
cd /mnt/c/github/HttpClient_ESP32_Lib

g++ -std=c++17 -DNATIVE_TEST -DARDUINO_ARCH_NATIVE \
    -Itest/helpers -Itest/fixtures -Isrc \
    test/unit/utils/test_utils.cpp \
    src/utils/Utils.cpp src/utils/HttpMethod.cpp \
    -lunity -o test_utils_runner

./test_utils_runner
```

### 方法3: 統合テスト（MockServer使用）

```bash
# MockServer起動
docker-compose up -d mockserver

# 統合テスト実行
~/.platformio/penv/bin/pio test -e test_integration
```

## 📊 各テスト環境の詳細

### 1. Native環境（test_unit）

| 項目 | 詳細 |
|------|------|
| **main.cpp** | `src/native/main.cpp` |
| **実行時間** | 1-5秒 |
| **依存関係** | Unity のみ |
| **テスト対象** | ライブラリロジック |

**src/native/main.cppの内容:**
```cpp
#ifdef NATIVE_TEST
#include <iostream>
int main() {
    std::cout << "Native Test Environment" << std::endl;
    return 0;
}
#endif
```

### 2. ESP32環境（m5stack-atom）

| 項目 | 詳細 |
|------|------|
| **main.cpp** | `src/esp32/main.cpp` |
| **実行時間** | 1-3分 |
| **依存関係** | Arduino、WiFi、実ネットワーク |
| **テスト対象** | 完全なE2E動作 |

**src/esp32/main.cppの内容:**
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
// WiFi接続、HTTP通信テストを含む完全なESP32テスト
```

## 🔧 現在の動作状況

### ✅ 動作確認済み

1. **ESP32実機テスト**
   ```bash
   ~/.platformio/penv/bin/pio test -e m5stack-atom --upload-port /dev/ttyUSB0
   ```
   - WiFi接続テスト
   - HTTP/HTTPS通信テスト
   - NTP時刻同期テスト

2. **フォルダ分割ビルド**
   ```bash
   ~/.platformio/penv/bin/pio run -e m5stack-atom    # ESP32用
   ~/.platformio/penv/bin/pio run -e test_unit       # Native用
   ```

### 🔄 調整中

1. **PlatformIOテスト検出**
   - 現在: `Collected 1 tests (*)` と表示されるが詳細テスト未検出
   - 回避策: 個別テストファイルの直接実行

## 🐛 トラブルシューティング

### 問題1: Arduino.h エラー（解決済み）

**解決策:** フォルダ分割によりNative環境ではESP32用コードを除外

```ini
# Native環境設定
build_src_filter = +<*> +<native/*> -<esp32/*>  ✅
```

### 問題2: テスト検出されない（調整中）

**現状:** PlatformIOが個別テストファイルを検出しない

**回避策1:** 個別実行
```bash
g++ -std=c++17 -DNATIVE_TEST test/unit/utils/test_utils.cpp -o test_runner
./test_runner
```

**回避策2:** ESP32実機テスト使用
```bash
./scripts/esp32_attach.sh
~/.platformio/penv/bin/pio test -e m5stack-atom --upload-port /dev/ttyUSB0
```

## 💡 実用的な開発フロー

### 推奨ワークフロー

1. **ライブラリコード変更**
2. **個別ユニットテスト実行** - ロジック確認
3. **ESP32実機テスト** - 完全動作確認
4. **コミット・プッシュ**

### 効率化コマンド

```bash
# ~/.bashrc に追加
alias pio='~/.platformio/penv/bin/pio'
alias esp32-attach='./scripts/esp32_attach.sh'
alias esp32-test='pio test -e m5stack-atom --upload-port /dev/ttyUSB0'
alias esp32-build='pio run -e m5stack-atom'
alias native-build='pio run -e test_unit'

# 使用例
esp32-attach    # ESP32接続
esp32-test      # 実機テスト（動作確認済み）
esp32-build     # ESP32ビルド
native-build    # Nativeビルド
```

## 📈 パフォーマンス比較

| 実行方法 | 時間 | 信頼性 | 使用シーン |
|----------|------|--------|------------|
| 個別ユニットテスト | 数秒 | 🟡 基本 | 開発中の素早い確認 |
| ESP32実機テスト | 1-3分 | 🟢 最高 | 最終動作確認 |
| 統合テスト | 10-30秒 | 🟠 中程度 | プロトコル層確認 |

## 🎯 今後の改善計画

### Phase 1: テスト検出修正

- [ ] PlatformIOテスト検出問題の解決
- [ ] `test_filter` 設定最適化
- [ ] テストファイル命名規則統一

### Phase 2: CI/CD統合

- [ ] GitHub Actions設定
- [ ] 自動ビルド・テスト実行
- [ ] 複数ESP32ボード対応

### Phase 3: 高度なテスト

- [ ] パフォーマンステスト
- [ ] メモリリークテスト
- [ ] 長期間動作テスト

## 📚 関連ドキュメント

### セットアップガイド
- **[WSL ESP32完全構築ガイド](./WSL_ESP32_COMPLETE_GUIDE.md)** - 環境構築手順
- **[ESP32開発用スクリプト](../scripts/README.md)** - 自動化ツール

### 実機テスト
- **[実機テスト手順書](./TESTING_PROCEDURE.md)** - ESP32テスト詳細
- **[トラブルシューティングガイド](./TROUBLESHOOTING_GUIDE.md)** - 問題解決

### 開発プロセス
- **[開発ワークフロー](./DEVELOPMENT_WORKFLOW.md)** - 日常開発フロー

---

**このガイドはフォルダ分割方式（方法2）を採用し、ESP32とNative環境で異なるmain.cppを使用する実装に基づいています。ESP32実機テストは完全に動作確認済みです。** 🚀