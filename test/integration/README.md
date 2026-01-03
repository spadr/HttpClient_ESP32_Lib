# Layer 2: Integration Tests

PlatformIO Native環境 + MockServerで実行されるHTTPプロトコルレベルのテストです。

## 実行方法

```bash
# MockServerを起動（Dockerが必要）
docker run -d -p 1080:1080 mockserver/mockserver

# 統合テストを実行
pio test -e native --filter "integration/*"

# 特定のシナリオテスト
pio test -e native --filter "*redirect*"
pio test -e native --filter "*ssl*"
pio test -e native --filter "*proxy*"
```

## 実装予定テスト

- HTTP/HTTPSプロトコル検証
- リダイレクト動作テスト
- エラーレスポンス処理
- SSL/TLS証明書検証
- プロキシ経由通信
- 認証機能テスト
- Cookie処理テスト
- チャンク転送エンコーディング

## MockServer設定例

```javascript
// 正常レスポンス
{
  "httpRequest": {
    "method": "GET",
    "path": "/api/users"
  },
  "httpResponse": {
    "statusCode": 200,
    "headers": {
      "Content-Type": ["application/json"]
    },
    "body": {
      "users": ["alice", "bob"]
    }
  }
}
```

## 特徴

- **実行時間**: 1-2分
- **外部依存**: MockServer（Docker）
- **対象**: HTTPプロトコル仕様準拠
- **エラーシナリオ**: 網羅的テスト