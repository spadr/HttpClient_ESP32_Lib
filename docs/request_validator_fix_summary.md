# RequestValidator機能の修正内容

## 修正日: 2025-01-21

### 修正した問題
1. **クライアントオプションの検証不足**
   - maxRedirects、maxRetriesの負値チェックが未実装
   - プロキシURLのスキーマ存在チェックが不足

2. **ヘッダー検証の未実装**
   - 空のヘッダー名の検証が未実装

3. **エラーコードの不一致**
   - サポートされていないプロトコルの場合、`InvalidURL`ではなく`UnsupportedProtocol`を返すべき

### 修正内容

#### RequestValidator.h
- `validateHeaders`メソッドを追加
- `validateBody`メソッドを追加

#### RequestValidator.cpp
1. `validate`メソッド:
   - ヘッダー検証とボディ検証の呼び出しを追加

2. `validateUrl`メソッド:
   - サポートされていないスキームの場合のエラーコードを`UnsupportedProtocol`に変更

3. `validateClientOptions`メソッド:
   - プロキシURLのスキーマ存在チェックを追加
   - maxRedirects < 0 の場合のエラーチェックを追加
   - maxRetries < 0 の場合のエラーチェックを追加

4. `validateHeaders`メソッド（新規追加）:
   - 空のヘッダー名をチェック

5. `validateBody`メソッド（新規追加）:
   - 現在は制約なし（GETリクエストでもボディを許可）

### テスト結果（期待値）
修正後、以下のテストが成功するはずです：
- `test_validate_unsupported_scheme`: FTPなどのサポートされていないプロトコルでUnsupportedProtocolエラー
- `test_validate_proxy_url_invalid`: 無効なプロキシURLでInvalidProxyURLエラー
- `test_validate_headers_empty_name`: 空のヘッダー名でInvalidHeaderエラー
- `test_validate_max_redirects_negative`: 負のmaxRedirectsでInvalidOptionエラー
- `test_validate_max_retries_negative`: 負のmaxRetriesでInvalidOptionエラー

### 注意事項
- GETリクエストでのボディ送信は現在許可されています（RFC的には非推奨ですが、一部のAPIで使用される場合があるため）
- Basic認証とBearer認証での空の認証情報は現在許可されています（テストの期待値に合わせて実装）