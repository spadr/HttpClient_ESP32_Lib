# E2E Test Worker Setup

このディレクトリには、HttpClient_ESP32_LibのE2Eテストで使用するCloudflare Workersのコードが含まれています。
`httpbin.org` の代わりとして、自前ドメイン（`e2e.canaspad.net`）でテストエンドポイントを運用するために使用します。

## セキュリティについて

本WorkerはAPIトークンによる簡易的な認証を実装しています。
リクエストヘッダーに `X-E2E-Token` が含まれていない、または環境変数と一致しない場合、`401 Unauthorized` を返します。

## セットアップ手順

### 1. Cloudflare Workersの準備
1. Cloudflareダッシュボードにログインし、"Workers & Pages" に移動します。
2. "Create Application" -> "Create Worker" を選択します。
3. Worker名（例: `httpclient-e2e-test`）を入力して作成します。

### 2. 環境変数の設定
1. 作成したWorkerの設定画面（Settings）を開きます。
2. "Variables" セクションで "Add Variable" をクリックします。
3. 以下の変数を設定します：
   - Variable name: `E2E_SECRET_TOKEN`
   - Value: (任意の推測されにくい文字列。例: `your-secret-token-12345`)
   - "Encrypt" ボタンをクリックして暗号化することを推奨します。
4. "Save and Deploy" をクリックします。

### 3. コードのデプロイ
1. Workerの "Edit Code" ボタンをクリックします。
2. 左側のエディタの内容を、このディレクトリにある `worker.js` の内容で完全に上書きします。
3. "Deploy" をクリックして保存・デプロイします。

### 4. ドメインの割り当て
1. CloudflareのWorkersダッシュボードに戻り、作成したWorkerの設定画面を開きます。
2. "Triggers" -> "Custom Domains" を選択します。
3. "Add Custom Domain" をクリックし、`e2e.canaspad.net` を入力します。
   （※ `canaspad.net` がCloudflareで管理されている必要があります）
4. DNSレコードが自動的に追加され、証明書が発行されるのを待ちます（数分かかる場合があります）。

## 使い方

### Worker側の設定
Cloudflareのダッシュボードで環境変数 `E2E_SECRET_TOKEN` が設定されていることを確認してください。

### クライアント（ESP32）側の設定
`src/Config.h` にトークンを設定します。

```cpp
namespace Config {
    // ...
    const char *e2e_token = "your-secret-token-12345";
}
```

テストコード内では以下のようにヘッダーを付与します：

```cpp
request.addHeader("X-E2E-Token", Config::e2e_token);
```

## 提供されるエンドポイント

| パス | 説明 | httpbin互換 |
|------|------|-------------|
| `/get` | リクエスト情報をJSONで返します | ✅ |
| `/post` | POSTデータをそのままJSONで返します | ✅ |
| `/status/{code}` | 指定されたHTTPステータスコードを返します | ✅ |
| `/delay/{n}` | 指定秒数（n秒）待機してからレスポンスを返します | ✅ |
| `/bytes/{n}` | 指定バイト数（nバイト）のバイナリデータを返します | ✅ |
| `/redirect/{n}` | 指定回数（n回）リダイレクトします | ✅ |
