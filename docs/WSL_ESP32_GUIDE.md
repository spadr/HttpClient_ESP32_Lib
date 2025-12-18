# WSL ESP32 USB接続ガイド

WSL環境でESP32開発ボードを使用するための簡単セットアップガイドです。

## 🚀 クイックスタート

### 1. 初回のみ - Windows側で共有設定

**PowerShell（管理者権限）で実行:**
```powershell
# USB デバイス一覧表示
usbipd list

# ESP32デバイスを共有（例: BUSID が 4-4 の場合）
usbipd bind --busid 4-4
```

### 2. WSLでESP32をアタッチ

```bash
# ESP32を自動検出してWSLにアタッチ
./scripts/esp32_attach.sh
```

### 3. PlatformIOでテスト実行

```bash
# ESP32テスト実行
./scripts/esp32_test_runner.sh

# または直接
pio test -e m5stack-atom
```

## 📋 詳細説明

以下の **`esp32_attach.sh`** が、WSL Ubuntu 上で――

* **Windows 側の USB‑UART ブリッジ（CP210x / CH34x / FTDI など）を自動検出**
* **usbipd win でアタッチ（未アタッチなら自動で attach）**
* **生成された `/dev/ttyUSB*` / `/dev/ttyACM*` を返す**

という一連の処理を行う Bash スクリプトです。

## 🔧 esp32_attach.sh の内容

スクリプトの内容は `scripts/esp32_attach.sh` に含まれています。

```bash
#!/usr/bin/env bash
# esp32_attach.sh ― ESP32 DevKit / M5Stack 自動検出 & usbipd attach
#   実行場所: WSL2 (Ubuntu) シェル
#   依存: usbipd‑win ≥ 4.0、lsusb、esptool.py(任意)

set -euo pipefail
shopt -s nocaseglob

# ――― 1. 代表的な USB‑UART ブリッジの VID:PID 一覧 ―――
VIDPIDS=(
  "10C4:EA60"   # Silicon‑Labs CP210x UART Bridge (多くの ESP32 DevKit)
  "1A86:7523"   # QinHeng CH340/CH341
  "0403:6010"   # FTDI FT2232H
  "0403:6011"   # FTDI FT4232H
  "0403:6014"   # FTDI FT232H
)

# ――― 2. Windows 側の USB 一覧を取得し、対象デバイスを抽出 ―――
echo "[INFO] Scanning Windows USB bus..."
mapfile -t LINES < <(usbipd list)      # 4.0 以降は `usbipd list` が推奨
for line in "${LINES[@]}"; do
  # 例行: "4-4    Silicon Labs CP2102 USB to UART Bridge (10C4:EA60)  Shared"
  if [[ $line =~ ^[[:space:]]*([0-9-]+)[[:space:]]+.*\(([0-9A-Fa-f]{4}):([0-9A-Fa-f]{4})\)[[:space:]]+([A-Za-z]+) ]]; then
    BUSID="${BASH_REMATCH[1]}"
    ID="${BASH_REMATCH[2]}:${BASH_REMATCH[3]}"
    STATE="${BASH_REMATCH[4]}"

    if printf '%s\n' "${VIDPIDS[@]}" | grep -iq "$ID"; then
      echo "  • Found candidate $BUSID ($ID) state:$STATE"
      if [[ "$STATE" == "Not"*"shared" ]] || [[ "$STATE" == "Shared" ]]; then
        # まだ attach されていなければ実行
        echo "    → Attaching to WSL..."
        usbipd attach --wsl --busid "$BUSID" || true   # 既に attach 済みでも OK
      fi
    fi
  fi
done

# ――― 3. /dev/tty デバイスが現れるまで待機 ―――
echo "[INFO] Waiting for /dev/ttyUSB* or /dev/ttyACM* ..."
for _ in {1..10}; do
  PORTS=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true)
  [[ -n "$PORTS" ]] && break
  sleep 1
done
[[ -z "$PORTS" ]] && { echo "[ERROR] Serial port not detected"; exit 1; }

echo "[INFO] Detected serial ports:"
for p in $PORTS; do
  echo "  $p"
done

# ――― 4. 任意：esptool でチップ確認 ―――
for p in $PORTS; do
  if command -v esptool.py &>/dev/null; then
    echo "[INFO] Probing $p ..."
    if esptool.py --port "$p" --before default_reset --after no_reset chip_id &>/dev/null; then
      echo "✅ ESP32 detected on $p"
      SELECTED="$p"
      break
    fi
  fi
done

# ――― 5. 出力 & 次の処理へ ―――
echo
echo "Use this port with PlatformIO:"
echo "   pio run -t upload --upload-port \"${SELECTED:-$PORTS}\""
```

## 📝 使い方詳細

### 1. usbipd-winのインストール

**方法1: wingetを使用**
```powershell
winget install --interactive --exact dorssel.usbipd-win
```

**方法2: GitHub Releasesから**
- [usbipd-win Releases](https://github.com/dorssel/usbipd-win/releases)から最新版をダウンロード
- .msiファイルを実行してインストール

### 2. 初回共有設定（PowerShell管理者権限）

```powershell
# USB デバイス一覧表示
usbipd list

# ESP32デバイスを見つけて共有
usbipd bind --busid <BUSID>   # 例: 4-4
```

共有は永続化されるので以降は不要です。([Microsoft Learn][1], [GitHub][2])

### 3. WSL でESP32アタッチ

```bash
# 自動アタッチスクリプト実行
./scripts/esp32_attach.sh
```

`/dev/ttyUSB0` などが表示されれば成功。

### 4. PlatformIO でテスト・開発

```bash
# テスト実行
pio test -e m5stack-atom

# プログラム書き込み
pio run -t upload --upload-port /dev/ttyUSB0
```

## 🔍 トラブルシューティング

### usbipd-winが見つからない
```bash
# エラー: usbipd: command not found
```
**解決策:** usbipd-winをインストールしてください（上記参照）

### Permission denied エラー
```bash
# /dev/ttyUSB0: Permission denied
```
**解決策:**
```bash
# ユーザーをdialoutグループに追加
sudo usermod -a -G dialout $USER
# ログアウト→ログインまたは
newgrp dialout
```

### ESP32デバイスが見つからない
- USBケーブルを確認
- 別のUSBポートを試す
- Windowsデバイスマネージャーでドライバー確認
- `usbipd list` で認識されているか確認

## 💡 補足情報

* VID/PID は Silicon‑Labs CP210x (`10C4:EA60`) と CH34x (`1A86:7523`) が圧倒的多数を占めます。
* `usbipd attach` は管理者権限不要ですが、`usbipd bind`（初回共有）だけは管理者権限が要ります。
* `esptool.py` が無い場合はスクリプトがチップ probe をスキップします。

## 🎯 スクリプトのロジック

`esp32_attach.sh` は以下の処理を自動実行:

1. **`usbipd list` をパース**し、`(VID:PID)` を正規表現で抽出
2. **ESP32 でよく使われるブリッジの VID/PID リストと突き合わせ**
3. `usbipd attach --wsl` で WSL にデバイスを引き込む
4. `/dev/ttyUSB*` などのノード生成を待機し、**最初に見つかったポートを返却**
5. 任意で **`esptool.py chip_id`** を実行して実機検出を二重確認

## 🚀 CI/自動化での利用

このスクリプトはCI環境でも利用可能です。複数のUSB-UARTデバイスが接続されていても、ESP32を自動特定して以下の流れを無人化できます：

**ESP32自動特定 → ビルド → 書き込み → テスト実行**

## 📚 参考リンク

- [Microsoft Learn - WSL USB接続](https://learn.microsoft.com/en-us/windows/wsl/connect-usb)
- [usbipd-win GitHub](https://github.com/dorssel/usbipd-win)
- [CP210x Device Hunt](https://devicehunt.com/view/type/usb/vendor/10C4/device/EA60)
- [CH340 Device Hunt](https://devicehunt.com/view/type/usb/vendor/1A86/device/7523)

---

**ESP32 on WSL Development Ready! 🎉**

[1]: https://learn.microsoft.com/en-us/windows/wsl/connect-usb "Connect USB devices | Microsoft Learn"
[2]: https://github.com/dorssel/usbipd-win/wiki/WSL-support "WSL support · dorssel/usbipd-win Wiki · GitHub"