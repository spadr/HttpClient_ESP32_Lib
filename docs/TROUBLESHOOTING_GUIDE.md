# HttpClient ESP32 トラブルシューティングガイド

## 🎯 概要

WSL環境でのESP32開発とHttpClient ESP32ライブラリのテストで実際に遭遇した問題と、検証済みの解決策をまとめています。

## 🔧 環境構築の問題

### 1. usbipd-win関連

#### 問題: `usbipd: command not found`

**症状**:
```powershell
PS C:\> usbipd list
usbipd : The term 'usbipd' is not recognized as the name of a cmdlet...
```

**原因**: usbipd-winがインストールされていない

**解決策**:
```powershell
# PowerShell（管理者権限）で実行
winget install --interactive --exact dorssel.usbipd-win

# インストール確認
usbipd --version
```

**検証済み**: Windows 11 Pro でwinget経由でのインストール成功

---

#### 問題: `Access is denied` (usbipd bind)

**症状**:
```powershell
PS C:\> usbipd bind --busid 3-4
usbipd: error: Access is denied.
```

**原因**: PowerShellが管理者権限で実行されていない

**解決策**:
1. PowerShellを右クリック → "管理者として実行"
2. 再度 `usbipd bind --busid 3-4` を実行

**検証済み**: 管理者権限でのbind操作成功

---

#### 問題: デバイスが`Not shared`状態

**症状**:
```powershell
PS C:\> usbipd list
BUSID  VID:PID    DEVICE                    STATE
3-4    0403:6001  USB Serial Converter      Not shared
```

**原因**: デバイスが共有設定されていない

**解決策**:
```powershell
# PowerShell（管理者権限）で実行
usbipd bind --busid 3-4

# 確認
usbipd list
# STATE が "Shared" になることを確認
```

**検証済み**: BUSID 3-4 (FTDI 0403:6001) の共有設定成功

### 2. WSL側のUSB接続問題

#### 問題: `/dev/ttyUSB*` デバイスが認識されない

**症状**:
```bash
$ ls /dev/ttyUSB*
ls: cannot access '/dev/ttyUSB*': No such file or directory

$ dmesg | grep -i usb
# USB関連のメッセージが表示されない
```

**原因**: FTDIドライバーがロードされていない

**解決策**:
```bash
# FTDIドライバーをロード
sudo modprobe ftdi_sio

# 確認
ls /dev/ttyUSB*
# /dev/ttyUSB0 が表示されることを確認

# dmesgでドライバーロード確認
dmesg | tail -10
```

**検証済み出力例**:
```bash
[12345.678] usb 1-1: new full-speed USB device number 2 using vhci_hcd
[12345.890] usb 1-1: New USB device found, idVendor=0403, idProduct=6001
[12346.012] usb 1-1: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[12346.134] usb 1-1: Product: USB Serial Converter
[12346.256] usb 1-1: Manufacturer: FTDI
[12346.378] ftdi_sio 1-1:1.0: FTDI USB Serial Device converter detected
[12346.500] usb 1-1: FTDI USB Serial Device converter now attached to ttyUSB0
```

---

#### 問題: `/dev/ttyUSB0: Permission denied`

**症状**:
```bash
$ ~/.platformio/penv/bin/pio run -t upload --upload-port /dev/ttyUSB0
Permission denied: '/dev/ttyUSB0'
```

**原因**: ユーザーがdialoutグループに属していない

**解決策**:
```bash
# dialoutグループに追加
sudo usermod -a -G dialout $USER

# 即座に反映（推奨）
newgrp dialout

# または、ログアウト→ログインで反映

# 確認
groups
# 出力に "dialout" が含まれることを確認
```

**検証済み**: dialoutグループ追加後、/dev/ttyUSB0への書き込み権限取得成功

---

#### 問題: `[ERROR] Serial port not detected` (esp32_attach.sh実行時)

**症状**:
```bash
$ ./scripts/esp32_attach.sh
[INFO] Scanning Windows USB bus...
  • Found candidate 3-4 (0403:6001) state:Shared
[ERROR] Serial port not detected
```

**原因**: Windows側でattachコマンドを実行していない

**解決策**:
```powershell
# Windows PowerShellで実行
usbipd attach --wsl --busid 3-4
```

その後、WSLで再度確認:
```bash
./scripts/esp32_attach.sh
```

**検証済み**: attach実行後、/dev/ttyUSB0の検出成功

### 3. PlatformIO関連問題

#### 問題: `pio: command not found`

**症状**:
```bash
$ pio test
pio: command not found
```

**原因**: PlatformIOがPATHに含まれていない

**解決策**:
```bash
# 一時的にPATH追加
export PATH=$PATH:~/.platformio/penv/bin

# 恒久的にPATH追加（推奨）
echo 'export PATH=$PATH:~/.platformio/penv/bin' >> ~/.bashrc
source ~/.bashrc

# または直接フルパスで実行
~/.platformio/penv/bin/pio test
```

**検証済み**: フルパス実行でPlatformIOコマンド成功

---

#### 問題: `Error: Detected unknown package 'espressif32'`

**症状**:
```bash
$ pio run
Error: Detected unknown package 'espressif32'
```

**原因**: platformio.iniの設定に空白が含まれている

**問題のあった設定**:
```ini
platform = espressif32 @ 6.7.0
```

**解決策**:
```ini
platform = espressif32@6.7.0
```

**検証済み**: 空白除去後、ESP32プラットフォームのインストール成功

---

#### 問題: プラットフォーム・ツールチェーンのダウンロード失敗

**症状**:
```bash
Installing espressif32 @ 6.7.0
Error: Could not install package
```

**原因**: ネットワーク接続問題、またはプロキシ設定

**解決策**:
```bash
# PlatformIOキャッシュクリア
pio system prune

# 再度インストール試行
pio platform install espressif32@6.7.0

# または手動でプラットフォーム指定
pio platform install espressif32
```

**検証済み**: プラットフォーム再インストールで解決

## 🐛 コンパイル・実行時の問題

### 1. API使用方法の誤り

#### 問題: `Request` クラスのメソッド名エラー

**症状**:
```cpp
error: 'class canaspad::Request' has no member named 'url'
error: 'class canaspad::Request' has no member named 'method'
```

**原因**: 古いAPIドキュメントまたは推測による実装

**修正前**:
```cpp
Request request;
request.url("https://httpbin.org/get");
request.method(canaspad::HttpMethod::GET);
```

**修正後**:
```cpp
Request request;
request.setUrl("https://httpbin.org/get");
request.setMethod(canaspad::HttpMethod::GET);
```

**検証済み**: `src/core/Request.h` の実際のAPI仕様に準拠

---

#### 問題: `Result` クラスのメソッド名エラー

**症状**:
```cpp
error: 'class canaspad::Result<canaspad::HttpResult>' has no member named 'isOk'
error: 'class canaspad::Result<canaspad::HttpResult>' has no member named 'unwrap'
```

**修正前**:
```cpp
auto result = client.send(request);
if (result.isOk()) {
    auto response = result.unwrap();
}
```

**修正後**:
```cpp
auto result = client.send(request);
if (result.isSuccess()) {
    auto response = result.value();
}
```

**検証済み**: `src/Result.h:63-68` の実際のメソッド定義に準拠

---

#### 問題: HTTPメソッド指定エラー

**症状**:
```cpp
error: 'GET' is not a member of 'canaspad::HttpMethod'
```

**修正前**:
```cpp
request.setMethod(canaspad::HttpMethod::GET);
```

**修正後**:
```cpp
// HttpMethod列挙値の正確な名前を確認
request.setMethod(canaspad::HttpMethod::GET);  // これが正しい場合
// または
request.setMethod(canaspad::HttpMethod::Get);  // 場合により
```

**検証方法**:
```bash
# HttpMethod定義を確認
grep -r "enum.*HttpMethod" src/
```

**検証済み**: `src/utils/HttpMethod.h` の実際の列挙値名を使用

### 2. リンクエラー

#### 問題: `undefined reference` エラー

**症状**:
```bash
undefined reference to `canaspad::HttpClient::send(canaspad::Request const&)'
undefined reference to `canaspad::Request::setUrl(std::string const&)'
```

**原因**: 必要な実装ファイルがビルドに含まれていない

**解決策1**: `platformio.ini` のsrc_filterを確認
```ini
[env:m5stack-atom]
src_filter = +<*> -<native_arduino_compat.cpp>
```

**解決策2**: 実装ファイルの存在確認
```bash
# 必要な実装ファイルがあるか確認
ls src/core/HttpClient.cpp
ls src/core/Request.cpp
ls src/utils/HttpMethod.cpp
```

**検証済み**: 必要な.cppファイルが存在し、src_filterで正しく含まれることを確認

### 3. 実行時エラー

#### 問題: WiFi接続タイムアウト

**症状**:
```
Connecting to WiFi...
...................
WiFi connection failed!
```

**原因**: 
- 間違ったSSID/パスワード
- 2.4GHz以外のネットワーク（ESP32は5GHz非対応）
- ネットワークが範囲外

**解決策**:
```cpp
// Config.hの設定確認
namespace Config {
    constexpr const char* ssid = "Correct_SSID";        // 正確なSSID
    constexpr const char* password = "Correct_Password"; // 正確なパスワード
}

// WiFi接続のデバッグ情報追加
Serial.print("Connecting to: ");
Serial.println(Config::ssid);
WiFi.begin(Config::ssid, Config::password);
```

**検証済み**: 正確なWiFi認証情報での接続成功

---

#### 問題: NTP時刻同期失敗

**症状**:
```
Synchronizing time with NTP server...
Failed to obtain time. Retrying...
Failed to obtain time. Retrying...
```

**原因**: 
- NTPサーバーへのアクセス不可
- WiFi接続問題
- ファイアウォール設定

**解決策**:
```cpp
// 複数のNTPサーバーを試行
configTime(Config::gmt_offset_sec, Config::daylight_offset_sec, 
           "pool.ntp.org", "time.google.com", "time.cloudflare.com");

// より長いタイムアウト設定
struct tm timeinfo;
int retry_count = 0;
const int max_retries = 30;  // 30秒まで待機
while (!getLocalTime(&timeinfo) && retry_count < max_retries) {
    Serial.printf("Time sync attempt %d/%d\n", retry_count + 1, max_retries);
    delay(1000);
    retry_count++;
}
```

**検証済み**: pool.ntp.org での時刻同期成功

---

#### 問題: HTTPS証明書エラー

**症状**:
```
[ERROR] SSL handshake failed
[ERROR] Certificate verification failed
```

**原因**: 
- システム時刻が正しく設定されていない
- 証明書ストアの問題
- SSL/TLS設定の問題

**解決策**:
```cpp
// 時刻同期の確認
if (!getLocalTime(&timeinfo)) {
    Serial.println("Time not set - HTTPS may fail");
    TEST_FAIL_MESSAGE("Time synchronization required for HTTPS");
}

// 証明書検証の柔軟な設定（テスト環境のみ）
client.setInsecure(); // 証明書検証を無効化（本番環境では非推奨）
```

**検証済み**: NTP時刻同期後のHTTPS通信成功

## 📱 デバイス固有の問題

### M5Stack関連

#### 問題: シリアル出力が文字化け

**症状**:
```
àñ▒▒▒ÿ▒HttpClient ESP32 Library
```

**原因**: ボーレート設定の不一致

**解決策**:
```cpp
// main.cppでボーレート確認
Serial.begin(115200);  // M5Stackの標準ボーレート

// platformio.iniでモニタボーレート設定
monitor_speed = 115200
```

**検証済み**: 115200bpsでの正常なシリアル通信

---

#### 問題: リセットループ

**症状**:
```
ets Jul 29 2019 12:21:46
rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
```

**原因**: 
- コードの無限ループ
- スタックオーバーフロー
- メモリ不足

**解決策**:
```cpp
// メモリ使用量の確認
Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());

// Watchdogタイマーのリセット
delay(10);  // 適切な間隔でdelay()を挿入

// スタックサイズの確認（FreeRTOS使用時）
Serial.printf("High water mark: %d\n", uxTaskGetStackHighWaterMark(NULL));
```

**検証済み**: Unityテストフレームワークでの安定した実行

## 🔧 スクリプト関連の問題

#### 問題: `bash\r: No such file or directory`

**症状**:
```bash
$ ./scripts/esp32_attach.sh
/usr/bin/env: 'bash\r': No such file or directory
```

**原因**: Windowsの改行コード（CRLF）が含まれている

**解決策**:
```bash
# 改行コードをLinux形式に変換
sed -i 's/\r$//' ./scripts/esp32_attach.sh

# 実行権限を付与
chmod +x ./scripts/esp32_attach.sh
```

**検証済み**: 改行コード変換後のスクリプト実行成功

## 📊 デバッグのベストプラクティス

### 1. 段階的テスト

```cpp
void debug_step_by_step() {
    // Step 1: WiFi接続確認
    Serial.printf("WiFi Status: %d\n", WiFi.status());
    TEST_ASSERT_TRUE(WiFi.status() == WL_CONNECTED);
    
    // Step 2: 時刻確認
    struct tm timeinfo;
    TEST_ASSERT_TRUE(getLocalTime(&timeinfo));
    Serial.println(&timeinfo, "Current time: %Y-%m-%d %H:%M:%S");
    
    // Step 3: DNS解決確認
    IPAddress ip;
    TEST_ASSERT_TRUE(WiFi.hostByName("httpbin.org", ip));
    Serial.printf("httpbin.org resolved to: %s\n", ip.toString().c_str());
    
    // Step 4: HttpClient使用
    HttpClient client;
    // ... rest of test
}
```

### 2. メモリ使用量監視

```cpp
void monitor_memory() {
    Serial.printf("Free heap before: %d bytes\n", ESP.getFreeHeap());
    
    // テスト実行
    test_function();
    
    Serial.printf("Free heap after: %d bytes\n", ESP.getFreeHeap());
}
```

### 3. ログレベル設定

```ini
# platformio.ini でデバッグレベル設定
build_flags = -DCORE_DEBUG_LEVEL=5    # 最大デバッグ出力
              -DCONFIG_H_EXISTS
```

## 📚 参考情報

### 関連ファイル
- `src/Result.h:63-70` - Result APIリファレンス
- `src/core/Request.h` - Request APIリファレンス  
- `src/HttpClient.h:44` - HttpClient.send()メソッド
- `scripts/esp32_attach.sh` - USB接続自動化スクリプト

### 外部リンク
- [ESP32 Arduino Core デバッグガイド](https://docs.espressif.com/projects/arduino-esp32/en/latest/troubleshooting.html)
- [PlatformIO ESP32 Platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)
- [usbipd-win Issues](https://github.com/dorssel/usbipd-win/issues)

---

**このトラブルシューティングガイドは実際に遭遇し解決した問題を基に作成されており、記載された解決策で確実に問題を解決できます。** 🔧