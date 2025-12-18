# HttpClient ESP32 開発用スクリプト

## 🎯 概要

WSL環境でのESP32開発を効率化するための検証済みスクリプト集です。すべてのスクリプトは実際のESP32実機テストで動作確認済みです。

## 🚀 利用可能なスクリプト

### ESP32開発用スクリプト

- **`esp32_attach.sh`** - ESP32 USBデバイスの自動検出とWSL接続
  - Windows USBバスをスキャンし、ESP32デバイスを自動検出
  - VID:PID パターンマッチングによる自動認識
  - `/dev/ttyUSB*` 生成まで自動待機

## 📋 実際の使用方法

### WSLユーザー向け（ESP32 + USB接続）

#### 初回セットアップ（一度のみ実行）

**Windows PowerShell（管理者権限）で実行:**

```powershell
# 1. usbipd-winインストール
winget install --interactive --exact dorssel.usbipd-win

# 2. ESP32デバイス確認
usbipd list

# 3. ESP32デバイス共有設定（例: BUSID 3-4）
usbipd bind --busid 3-4
```

#### 日常の開発フロー

**毎回の作業開始時:**

```powershell
# Windows PowerShell - ESP32をWSLに接続
usbipd attach --wsl --busid 3-4
```

**WSL Ubuntu で実行:**

```bash
# 1. ESP32の自動検出・接続確認
./scripts/esp32_attach.sh

# 期待される出力:
# [INFO] Scanning Windows USB bus...
#   • Found candidate 3-4 (0403:6001) state:Shared
#     → Attaching to WSL...
# [INFO] Detected serial ports:
#   /dev/ttyUSB0

# 2. PlatformIOテスト実行
~/.platformio/penv/bin/pio test -e m5stack-atom --upload-port /dev/ttyUSB0

# 3. プログラム書き込み
~/.platformio/penv/bin/pio run -t upload --upload-port /dev/ttyUSB0
```

## 🔧 esp32_attach.sh の動作詳細

### 自動検出対象デバイス

スクリプトは以下のVID:PIDパターンでESP32デバイスを検出します：

- `10C4:EA60` - Silicon Labs CP210x UART Bridge（ESP32-DevKitC等）
- `1A86:7523` - QinHeng CH340/CH341（安価なESP32ボード）
- `0403:6001` - FTDI USB Serial Converter（M5Stack等）

### 実行フロー

1. **Windows USBバススキャン**
   ```bash
   powershell.exe -Command "usbipd list"
   ```

2. **ESP32デバイス検出**
   - VID:PIDパターンマッチング
   - デバイス状態確認（Shared/Not shared）

3. **自動WSL接続**
   ```bash
   powershell.exe -Command "usbipd attach --wsl --busid <検出されたBUSID>"
   ```

4. **シリアルポート待機**
   - `/dev/ttyUSB*` または `/dev/ttyACM*` の生成を10秒間待機
   - デバイス検出後、PlatformIOで使用可能なポート情報を表示

5. **ESP32チップ確認**（esptool利用可能時）
   ```bash
   esptool.py --port /dev/ttyUSB0 chip_id
   ```

## 📊 動作確認済み環境

- **Windows**: Windows 11 Pro
- **WSL**: Ubuntu 22.04 LTS  
- **usbipd-win**: バージョン 4.x
- **PlatformIO**: バージョン 6.1.18
- **ESP32デバイス**: M5Stack-Atom（FTDI FT232BM チップ使用）
- **VID:PID**: 0403:6001 (FTDI USB Serial Converter)

## 🚨 よくある問題と解決策

### 問題1: `[ERROR] Serial port not detected`

**原因**: Windows側でattachコマンドを実行していない

**解決策**:
```powershell
# Windows PowerShell で実行
usbipd attach --wsl --busid 3-4
```

### 問題2: `bash\r: No such file or directory`

**原因**: Windowsの改行コード（CRLF）

**解決策**:
```bash
sed -i 's/\r$//' ./scripts/esp32_attach.sh
chmod +x ./scripts/esp32_attach.sh
```

### 問題3: `/dev/ttyUSB0: Permission denied`

**原因**: dialoutグループ権限不足

**解決策**:
```bash
sudo usermod -a -G dialout $USER
newgrp dialout  # 即座に反映
```

## 💡 効率化のヒント

### エイリアス設定

`.bashrc` に追加することで、コマンドを短縮できます：

```bash
# ESP32開発用エイリアス
alias pio='~/.platformio/penv/bin/pio'
alias esp32-attach='./scripts/esp32_attach.sh'
alias esp32-test='pio test -e m5stack-atom --upload-port /dev/ttyUSB0'
alias esp32-upload='pio run -t upload --upload-port /dev/ttyUSB0'
alias esp32-monitor='pio device monitor --port /dev/ttyUSB0 --baud 115200'
```

### 自動起動設定（direnv使用時）

プロジェクトディレクトリに `.envrc` ファイルを作成：

```bash
# PlatformIOのPATH設定
export PATH=$PATH:~/.platformio/penv/bin

# ESP32デバイス自動検出
if [ -f "./scripts/esp32_attach.sh" ]; then
    echo "ESP32 attachment script available: ./scripts/esp32_attach.sh"
fi
```

## 📚 詳細ドキュメント

### 完全セットアップガイド
- **[WSL ESP32 完全構築ガイド](../docs/WSL_ESP32_COMPLETE_GUIDE.md)** - 初回環境構築からテスト実行まで
- **[実機テスト手順書](../docs/TESTING_PROCEDURE.md)** - テストコードの詳細と実際の動作
- **[トラブルシューティングガイド](../docs/TROUBLESHOOTING_GUIDE.md)** - 実際に遭遇した問題と解決策

### 開発ワークフロー
- **[開発ワークフロー](../docs/DEVELOPMENT_WORKFLOW.md)** - 日常的な開発フロー

## ✨ スクリプトの特徴

- **完全自動化**: ESP32デバイスの検出からWSL接続まで自動実行
- **エラーハンドリング**: 詳細なエラーメッセージと解決策の提示
- **実機検証済み**: 実際のESP32開発で動作確認済み
- **複数デバイス対応**: 主要なESP32開発ボードのVID:PIDに対応
- **状態管理**: デバイスの共有状態、接続状態を自動判定

---

**これらのスクリプトは実際のWSL ESP32開発環境で検証済みであり、記載された手順で確実にESP32開発環境を構築・運用できます。** 🚀