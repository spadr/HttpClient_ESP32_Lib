# Layer 3: End-to-End Tests

ESP32実機 + 実サーバーでの最終検証テストです。

## 実行方法

```bash
# ESP32実機でE2Eテストを実行
pio test -e m5stack-atom --filter "e2e/*"

# 特定の実機テスト
pio test -e m5stack-atom --filter "*performance*"
pio test -e m5stack-atom --filter "*memory*"
```

## テスト対象

- 実際のHTTPサーバーとの通信
- ESP32のメモリ制約下での動作
- WiFi接続の安定性
- パフォーマンス測定
- 長時間稼働テスト
- 実際のAPI（GitHub API等）との接続

## 設定

```cpp
// test/e2e/Config.h
namespace Config {
    const char* ssid = "your-wifi-ssid";
    const char* password = "your-wifi-password";
    const char* test_server = "httpbin.org";
    const char* ntp_host = "pool.ntp.org";
}
```

## パフォーマンス目標

- **メモリ使用量**: < 50KB
- **接続時間**: < 3秒
- **リクエスト処理時間**: < 1秒
- **スループット**: > 10 req/min

## 特徴

- **実行時間**: 5-15分
- **外部依存**: WiFi、実サーバー
- **ハードウェア**: ESP32実機必須
- **実環境**: 本番環境相当