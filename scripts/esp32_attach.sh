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
# WSL から Windows の usbipd を呼び出す
mapfile -t LINES < <(powershell.exe -Command "usbipd list" 2>/dev/null)
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
        powershell.exe -Command "usbipd attach --wsl --busid $BUSID" 2>/dev/null || true   # 既に attach 済みでも OK
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