# WSL ESP32開発環境完全構築ガイド

## 🎯 概要

このドキュメントは、WSL（Windows Subsystem for Linux）環境でESP32マイコンを使用したHttpClient ESP32ライブラリの開発・テスト環境を構築するための**実際に検証済みの完全な手順**です。

## 📋 前提条件

- Windows 10/11 with WSL2
- Ubuntu 22.04 (WSL2上)
- ESP32開発ボード（M5Stack、DevKit等）
- USBケーブル

## 🚀 完全セットアップ手順

### Step 1: usbipd-winのインストール（Windows側）

**PowerShell（管理者権限）で実行:**

```powershell
# 方法1: wingetを使用（推奨）
winget install --interactive --exact dorssel.usbipd-win

# 方法2: 手動インストール
# https://github.com/dorssel/usbipd-win/releases から最新版.msiをダウンロード・実行
```

**インストール確認:**
```powershell
usbipd --version
# usbipd 4.x.x などが表示されれば成功
```

### Step 2: ESP32デバイスの初回共有設定

**ESP32をUSBで接続後、PowerShell（管理者権限）で:**

```powershell
# 接続されたUSBデバイス一覧表示
usbipd list
```

**出力例:**
```
Connected:
BUSID  VID:PID    DEVICE                                                        STATE
3-4    0403:6001  USB Serial Converter                                          Not shared
```

**ESP32デバイスを共有:**
```powershell
# BUSID 3-4 を共有（実際のBUSIDに置き換え）
usbipd bind --busid 3-4
```

> **重要**: この設定は永続化されるため、**初回のみ**実行すれば十分です。

### Step 3: WSL側の準備

**WSL Ubuntu で実行:**

```bash
# PlatformIOインストール（未インストールの場合）
pip install platformio

# PATHに追加（.bashrcに追加推奨）
export PATH=$PATH:~/.platformio/penv/bin

# FTDIドライバー確認・ロード
sudo modprobe ftdi_sio

# dialoutグループに追加（権限設定）
sudo usermod -a -G dialout $USER
# ログアウト→ログインが必要（または newgrp dialout で即座に反映）
```

### Step 4: ESP32の自動接続スクリプト使用

**プロジェクトディレクトリで:**

```bash
cd /mnt/c/github/HttpClient_ESP32_Lib

# ESP32自動検出・接続スクリプト実行
./scripts/esp32_attach.sh
```

**成功時の出力例:**
```
[INFO] Scanning Windows USB bus...
  • Found candidate 3-4 (0403:6001) state:Shared
    → Attaching to WSL...
[INFO] Waiting for /dev/ttyUSB* or /dev/ttyACM* ...
[INFO] Detected serial ports:
  /dev/ttyUSB0

Use this port with PlatformIO:
   pio run -t upload --upload-port "/dev/ttyUSB0"
```

### Step 5: PlatformIOテスト実行

```bash
# ESP32実機テスト実行
~/.platformio/penv/bin/pio test -e m5stack-atom --upload-port /dev/ttyUSB0
```

## 🔧 実際に検証済みのトラブルシューティング

### 問題1: `usbipd: command not found`

**原因**: usbipd-winがインストールされていない  
**解決策**: Step 1を実行

### 問題2: ESP32デバイスが`/dev/ttyUSB*`に現れない

**原因**: FTDIドライバーがロードされていない  
**解決策**:
```bash
sudo modprobe ftdi_sio
# 必要に応じて以下も実行
sudo modprobe usbserial
```

### 問題3: `/dev/ttyUSB0: Permission denied`

**原因**: ユーザーがdialoutグループに属していない  
**解決策**:
```bash
sudo usermod -a -G dialout $USER
# 以下のいずれかで反映
newgrp dialout           # 即座に反映
# または ログアウト→ログイン
```

### 問題4: PlatformIOが見つからない

**原因**: PATHが通っていない  
**解決策**:
```bash
# 一時的にPATH追加
export PATH=$PATH:~/.platformio/penv/bin

# 恒久的にPATH追加（推奨）
echo 'export PATH=$PATH:~/.platformio/penv/bin' >> ~/.bashrc
source ~/.bashrc
```

### 問題5: PowerShellで`attach`コマンドを忘れた

**症状**: スクリプト実行時に`[ERROR] Serial port not detected`  
**解決策**:
```powershell
# Windows PowerShellで実行
usbipd attach --wsl --busid 3-4
```

## 🎯 実際に動作した完全ワークフロー

### 日常の開発フロー

**1. ESP32接続（毎回）**
```powershell
# Windows PowerShell
usbipd attach --wsl --busid 3-4
```

**2. WSLでの作業**
```bash
# 自動検出・接続確認
./scripts/esp32_attach.sh

# テスト実行
~/.platformio/penv/bin/pio test -e m5stack-atom --upload-port /dev/ttyUSB0

# プログラム書き込み
~/.platformio/penv/bin/pio run -t upload --upload-port /dev/ttyUSB0
```

**3. 作業終了時（オプション）**
```powershell
# Windows PowerShell（デバイス解放）
usbipd detach --busid 3-4
```

## 📊 動作確認済み環境

- **Windows**: Windows 11 Pro
- **WSL**: Ubuntu 22.04 LTS
- **usbipd-win**: バージョン 4.x
- **PlatformIO**: バージョン 6.1.18
- **ESP32デバイス**: M5Stack（FTDI FT232BM）
- **VID:PID**: 0403:6001 (FTDI USB Serial Converter)

## 🔍 esp32_attach.shスクリプトの動作詳細

スクリプトは以下の処理を自動実行します：

1. **Windows USB バススキャン**: `powershell.exe -Command "usbipd list"`
2. **ESP32デバイス自動検出**: VID:PID パターンマッチング
   - `10C4:EA60` (Silicon Labs CP210x)
   - `1A86:7523` (QinHeng CH340)
   - `0403:6001` (FTDI FT232)
3. **自動アタッチ**: `powershell.exe -Command "usbipd attach --wsl --busid <BUSID>"`
4. **デバイス待機**: `/dev/ttyUSB*` `/dev/ttyACM*` の生成を10秒間待機
5. **esptool確認**: 利用可能な場合、ESP32チップの検証実行

## 💡 開発効率化のヒント

### エイリアス設定

```bash
# ~/.bashrcに追加
alias pio='~/.platformio/penv/bin/pio'
alias esp32-test='pio test -e m5stack-atom --upload-port /dev/ttyUSB0'
alias esp32-upload='pio run -t upload --upload-port /dev/ttyUSB0'
alias esp32-attach='./scripts/esp32_attach.sh'
```

### 自動起動設定

```bash
# プロジェクトディレクトリの.envrcファイル（direnv使用時）
export PATH=$PATH:~/.platformio/penv/bin
```

## 🎉 期待される結果

正常にセットアップが完了すると：

1. **ESP32デバイス認識**: `/dev/ttyUSB0` として認識
2. **PlatformIO連携**: シームレスなビルド・アップロード
3. **テスト実行**: WiFi接続、HTTP通信テストの実行
4. **開発効率**: Windows環境を汚さないLinux開発環境

## 📚 関連ファイル

- `scripts/esp32_attach.sh` - ESP32自動接続スクリプト
- `test/main.cpp` - ESP32実機テストコード
- `src/Config.h` - WiFi設定ファイル
- `platformio.ini` - PlatformIO環境設定

## 🔗 参考リンク

- [Microsoft Learn - WSL USB接続](https://learn.microsoft.com/en-us/windows/wsl/connect-usb)
- [usbipd-win GitHub](https://github.com/dorssel/usbipd-win)
- [PlatformIO ESP32 Platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)

---

**このガイドは実際の構築・検証作業を基に作成されており、記載された手順で確実にWSL ESP32開発環境を構築できます。** ✨