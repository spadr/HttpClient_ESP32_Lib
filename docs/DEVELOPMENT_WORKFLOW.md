　# HttpClient ESP32 開発ワークフロー

## 🎯 概要

WSL環境でのHttpClient ESP32ライブラリ開発における日常的なワークフローを、実際の開発で検証済みの手順に基づいて説明します。

## 🚀 開発環境セットアップ

### 初回セットアップ（一度のみ）

#### 1. WSL ESP32環境構築

詳細手順は [WSL ESP32完全構築ガイド](./WSL_ESP32_COMPLETE_GUIDE.md) を参照してください。

```bash
# 要約版セットアップ
# 1. Windows側: usbipd-win インストール & ESP32デバイス共有
# 2. WSL側: PlatformIO インストール & ドライバー設定
# 3. プロジェクト設定: platformio.ini & test/Config.h 作成
```

#### 2. 開発効率化設定

**~/.bashrc にエイリアス追加:**

```bash
# ESP32開発用エイリアス
alias pio='~/.platformio/penv/bin/pio'
alias esp32-attach='./scripts/esp32_attach.sh'
alias esp32-test='pio test -e m5stack-atom --upload-port /dev/ttyUSB0'
alias esp32-upload='pio run -t upload --upload-port /dev/ttyUSB0'
alias esp32-monitor='pio device monitor --port /dev/ttyUSB0 --baud 115200'
alias esp32-clean='pio run -t clean'

# 汎用コマンド
alias ll='ls -la'
alias grep='grep --color=auto'
```

**設定反映:**

```bash
source ~/.bashrc
```

## 📅 日常の開発フロー

### 毎朝の作業開始時

#### 1. ESP32デバイス接続

**Windows PowerShell（通常権限で可）:**

```powershell
# ESP32をWSLに接続
usbipd attach --wsl --busid 3-4
```

**WSL Ubuntu:**

```bash
# プロジェクトディレクトリに移動
cd /mnt/c/github/HttpClient_ESP32_Lib

# ESP32接続確認
esp32-attach

# 期待される出力:
# [INFO] Detected serial ports:
#   /dev/ttyUSB0
```

#### 2. 開発環境確認

```bash
# Git状態確認
git status

# ビルド環境確認
pio run -t clean
pio run --dry-run
```

### 機能開発サイクル

#### 1. 新機能の実装

```bash
# 新しいブランチ作成（例: プロキシ機能追加）
git checkout -b feature/proxy-support

# 実装作業
# - src/ ディレクトリでライブラリコード編集
# - test/ ディレクトリでテストコード作成
```

#### 2. ユニットテスト実行

```bash
# ネイティブ環境でのユニットテスト（高速）
pio test -e native

# 期待される結果:
# Test Summary: 15 Tests 0 Failures 0 Ignored
```

#### 3. ESP32実機テスト

```bash
# ESP32デバイス接続確認
esp32-attach

# ESP32実機テスト実行
esp32-test

# 期待される結果:
# === HttpClient ESP32 Library - E2E Tests ===
# WiFi connected!
# 3 Tests 0 Failures 0 Ignored
```

#### 4. コード品質チェック

```bash
# コンパイル警告チェック
pio run -e m5stack-atom

# 静的解析（利用可能な場合）
pio check

# フォーマット確認（clang-format等使用時）
find src/ -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run
```

### デバッグワークフロー

#### 1. シリアルモニタでのデバッグ

```bash
# プログラム書き込み
esp32-upload

# シリアルモニタ開始
esp32-monitor

# 期待されるデバッグ出力:
# [  1234][D][WiFiGeneric.cpp:929] _eventCallback(): Event: 4 - STA_CONNECTED
# [  2345][D][HttpClient.cpp:123] send(): Sending request to https://httpbin.org/get
```

#### 2. 段階的デバッグ

**test/main.cpp でのデバッグコード例:**

```cpp
void debug_network_connection() {
    // 段階1: WiFi接続確認
    Serial.printf("WiFi Status: %d\n", WiFi.status());
    TEST_ASSERT_TRUE(WiFi.status() == WL_CONNECTED);
    
    // 段階2: DNS解決確認
    IPAddress ip;
    bool dns_ok = WiFi.hostByName("httpbin.org", ip);
    Serial.printf("DNS Resolution: %s -> %s\n", 
                  dns_ok ? "OK" : "FAILED", 
                  dns_ok ? ip.toString().c_str() : "N/A");
    TEST_ASSERT_TRUE(dns_ok);
    
    // 段階3: メモリ使用量確認
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // 段階4: HttpClient テスト
    HttpClient client;
    Request request;
    request.setUrl("https://httpbin.org/get");
    request.setMethod(canaspad::HttpMethod::GET);
    
    auto result = client.send(request);
    if (result.isSuccess()) {
        Serial.printf("HTTP Request: SUCCESS (Status: %d)\n", 
                      result.value().statusCode);
    } else {
        Serial.printf("HTTP Request: FAILED (%s)\n", 
                      result.error().message.c_str());
    }
}
```

#### 3. パフォーマンス測定

```cpp
void performance_test() {
    unsigned long start_time = millis();
    
    HttpClient client;
    Request request;
    request.setUrl("https://httpbin.org/get");
    request.setMethod(canaspad::HttpMethod::GET);
    
    auto result = client.send(request);
    
    unsigned long end_time = millis();
    unsigned long duration = end_time - start_time;
    
    Serial.printf("Request completed in %lu ms\n", duration);
    Serial.printf("Free heap after request: %d bytes\n", ESP.getFreeHeap());
    
    TEST_ASSERT_TRUE(result.isSuccess());
    TEST_ASSERT_TRUE(duration < 5000); // 5秒以内での応答
}
```

### コミット & プッシュワークフロー

#### 1. 変更内容の確認

```bash
# 変更されたファイル確認
git status

# 差分確認
git diff

# ステージング
git add src/ test/ docs/
```

#### 2. 最終テスト実行

```bash
# ネイティブテスト
pio test -e native

# ESP32実機テスト
esp32-test

# ビルド確認
pio run -e m5stack-atom
```

#### 3. コミット作成

```bash
# コミット作成
git commit -m "add: プロキシサポート機能を追加

- ProxyConnection クラス実装
- HTTP CONNECT メソッド対応  
- プロキシ認証機能追加
- E2Eテストでプロキシ接続確認

テスト結果:
- ユニットテスト: 18 Tests 0 Failures
- ESP32実機テスト: 5 Tests 0 Failures"

# プッシュ
git push origin feature/proxy-support
```

### 夕方の作業終了時

#### 1. 作業状況の保存

```bash
# 未完了の変更を一時保存
git stash push -m "WIP: プロキシタイムアウト処理実装中"

# またはコミット
git add .
git commit -m "WIP: プロキシタイムアウト処理の実装途中"
```

#### 2. 環境のクリーンアップ

```bash
# ビルドファイルクリーンアップ
pio run -t clean

# 一時ファイル削除
find . -name "*.tmp" -delete
find . -name "*.log" -delete
```

#### 3. ESP32デバイス解放（オプション）

**Windows PowerShell:**

```powershell
# ESP32デバイスをWSLから解放
usbipd detach --busid 3-4
```

## 🔄 継続的インテグレーション

### GitHub Actions との連携

**プッシュ時の自動チェック:**

1. **ネイティブビルド**: コンパイルエラーの早期発見
2. **ユニットテスト**: ロジックの正確性確認
3. **静的解析**: コード品質チェック
4. **ドキュメント生成**: API文書の自動更新

### ローカルでのCI模擬

```bash
# CIで実行されるチェックをローカル実行
pio run -e native                    # ネイティブビルド
pio test -e native                   # ユニットテスト
pio run -e m5stack-atom             # ESP32ビルド
pio check                           # 静的解析（利用可能時）
```

## 📊 開発メトリクス

### 日次メトリクス

```bash
# コード行数確認
find src/ -name "*.cpp" -o -name "*.h" | xargs wc -l

# テストカバレッジ（利用可能時）
pio test -e native --with-coverage

# ビルドサイズ確認
pio run -e m5stack-atom -v | grep "RAM\|Flash"
```

### 週次レビュー

- **パフォーマンス**: 実機テストの応答時間測定
- **メモリ使用量**: ESP32でのヒープ使用量監視
- **テストカバレッジ**: 新機能のテスト追加状況
- **ドキュメント**: 新機能の文書化状況

## 🚨 よくある開発中の問題

### 1. WiFi接続エラー

**現象**: テスト実行時にWiFi接続失敗

**対応**:
```cpp
// test/Config.h の設定確認
namespace Config {
    constexpr const char* ssid = "正確なSSID";
    constexpr const char* password = "正確なパスワード";
}
```

### 2. メモリ不足エラー

**現象**: ESP32実行時にリセットループ

**対応**:
```cpp
// メモリ使用量監視
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

// 大きなオブジェクトのスコープ制限
{
    HttpClient client;  // スコープを制限
    // ... テスト実行
}  // ここでclientが解放される
```

### 3. シリアル通信エラー

**現象**: `/dev/ttyUSB0: Permission denied`

**対応**:
```bash
# 権限確認・修復
sudo usermod -a -G dialout $USER
newgrp dialout

# デバイス状態確認
ls -la /dev/ttyUSB*
```

## 📚 参考リソース

### 内部ドキュメント
- [WSL ESP32完全構築ガイド](./WSL_ESP32_COMPLETE_GUIDE.md)
- [実機テスト手順書](./TESTING_PROCEDURE.md)
- [トラブルシューティングガイド](./TROUBLESHOOTING_GUIDE.md)

### 外部リソース
- [PlatformIO ESP32 Platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)
- [ESP32 Arduino Core API](https://docs.espressif.com/projects/arduino-esp32/)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)

### 開発ツール
- **Visual Studio Code** + PlatformIO IDE拡張
- **Serial Monitor**: ESP32デバッグ用
- **Git**: バージョン管理
- **GitHub Actions**: CI/CD

---

**この開発ワークフローは実際のHttpClient ESP32ライブラリ開発で使用し、効果を確認した手順です。記載された方法により効率的で安定した開発を行えます。** 🚀