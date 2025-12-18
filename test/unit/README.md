# Layer 1: Unit Tests

PlatformIO Native環境で実行される高速ユニットテストです。

## 実行方法

```bash
# すべてのネイティブテストを実行
pio test -e native

# 特定のテストを実行
pio test -e native --filter "*utils*"
pio test -e native --filter "*request_validator*"

# 詳細出力
pio test -e native -v
```

## テストファイル

- `test_utils.cpp` - Utilsクラスの関数テスト
- `test_request_validator.cpp` - RequestValidatorのテスト
- `test_httpclient_basic.cpp` - HttpClientの基本機能テスト（開発中）

## 対象範囲

- ✅ URL解析機能
- ✅ Base64エンコード
- ✅ リクエストバリデーション
- ⏳ HTTPクライアント基本機能
- ⏳ エラーハンドリング
- ⏳ モック通信

## 特徴

- **実行時間**: 数秒
- **Arduino依存**: なし（mockを使用）
- **外部依存**: なし
- **CI/CD対応**: GitHub Actions対応