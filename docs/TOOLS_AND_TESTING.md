# HttpClient ESP32 Library - ツールとテスト実行ガイド

## 目次
1. [クイックスタート](#クイックスタート)
2. [テストアーキテクチャ](#テストアーキテクチャ)
3. [テスト実行方法](#テスト実行方法)
4. [開発ツール](#開発ツール)
5. [トラブルシューティング](#トラブルシューティング)

## クイックスタート

### 最速でテストを実行する3ステップ

```bash
# 1. 開発環境をセットアップ
./tools/setup/setup_dev_env.sh

# 2. Dockerサービスを起動
cd docker && docker-compose -f test-services.yml up -d && cd ..

# 3. 全テストを実行
./scripts/run-tests.sh -l all
```

## テストアーキテクチャ

本プロジェクトは3層のテスト戦略を採用しています：

| レイヤー | 説明 | 環境 | 依存関係 |
|---------|------|------|----------|
| Layer 1 | ユニットテスト | Native | なし |
| Layer 2 | 統合テスト | Native + MockServer | Docker |
| Layer 3 | E2Eテスト | ESP32実機 | 実サーバー |

## テスト実行方法

### PlatformIOを使用（推奨）

```bash
# Layer 1: ユニットテスト
pio test -e native

# Layer 2: 統合テスト
pio test -e native_integration

# Layer 3: E2Eテスト
pio test -e m5stack-atom --filter "e2e/*"

# 特定のカテゴリのみ実行
pio test -e native --filter "unit/cookie/*"    # Cookieテストのみ
pio test -e native --filter "unit/auth/*"      # 認証テストのみ
pio test -e native --filter "unit/utils/*"     # ユーティリティテストのみ
```

### スクリプトを使用

#### 高機能テストランナー（推奨）
```bash
# 全テスト実行
./scripts/run-tests.sh -l all

# 特定レイヤーのみ実行
./scripts/run-tests.sh -l 1        # ユニットテストのみ
./scripts/run-tests.sh -l 2        # 統合テストのみ
./scripts/run-tests.sh -l 3        # E2Eテストのみ

# オプション
./scripts/run-tests.sh -l all -v   # 詳細出力
./scripts/run-tests.sh -l all -c   # クリーンビルド
```

#### Dockerローカルテスト
```bash
# 通常実行
./scripts/test-local.sh

# レコーディングモード（E2Eテスト用）
./scripts/test-local.sh --record
```

### 簡易テストランナー
```bash
# 全テスト実行（PlatformIOなしでも動作）
./tools/testing/run_all_tests.sh
```

### 手動コンパイル（PlatformIOなしの場合）

```bash
# 例：Utilsテスト
g++ -std=c++17 -DNATIVE_TEST -DARDUINO_ARCH_NATIVE \
    -I src -I test/helpers \
    test/unit/utils/test_utils.cpp \
    src/utils/Utils.cpp \
    src/utils/HttpMethod.cpp \
    src/native_arduino_compat.cpp \
    -o test_utils && ./test_utils
```

## 開発ツール

### Dockerサービス管理

```bash
# サービス起動
cd docker
docker-compose -f test-services.yml up -d

# サービス状態確認
docker-compose -f test-services.yml ps

# ログ確認
docker-compose -f test-services.yml logs -f mockserver

# サービス停止
docker-compose -f test-services.yml down
```

### 利用可能なモックサービス

| サービス | ポート | 用途 |
|---------|--------|------|
| MockServer | 1080 | HTTPモック（プログラマブル） |
| WireMock | 8080 | HTTPモック（代替） |
| Squid Proxy | 3128 | プロキシテスト用 |

### 開発環境セットアップ

```bash
# 完全セットアップ（PlatformIO + Docker + 依存関係）
./tools/setup/setup_dev_env.sh

# PlatformIOのみインストール
python3 tools/setup/get-platformio.py

# PlatformIO検証
python3 tools/validation/validate_platformio.py
```

### ビルド検証

```bash
# ビルド修正の検証
python3 tools/validation/verify_build_fixes.py

# インクルードチェック
python3 tools/validation/check_includes.py

# テスト重複チェック
python3 tools/validation/check_test_duplicates.py

# テストインフラストラクチャ確認
python3 tools/testing/test_runner.py
```

## トラブルシューティング

### よくある問題と解決方法

#### 1. MockServerに接続できない
```bash
# Dockerサービスの状態を確認
docker-compose -f docker/test-services.yml ps

# サービスを再起動
docker-compose -f docker/test-services.yml restart mockserver

# ポートの使用状況を確認
lsof -i :1080  # MacOS/Linux
netstat -an | findstr :1080  # Windows
```

#### 2. PlatformIOが見つからない
```bash
# PATHに追加
export PATH=$PATH:~/.platformio/penv/bin

# または再インストール
python3 tools/setup/get-platformio.py
```

#### 3. コンパイルエラー
```bash
# 依存関係をクリーンアップ
pio run -t clean

# ライブラリを再インストール
pio lib install
```

#### 4. テストがタイムアウトする
```bash
# タイムアウト値を増やす（platformio.iniで設定）
test_speed = 115200
test_timeout = 30  # 30秒に増加
```

### デバッグモード

```bash
# 詳細ログを有効化
export PLATFORMIO_TEST_VERBOSE=1
pio test -e native -v

# 特定のテストのみ実行してデバッグ
pio test -e native --filter "unit/cookie/test_cookie" -v
```

### 環境変数

| 変数名 | 説明 | デフォルト値 |
|--------|------|-------------|
| `PLATFORMIO_TEST_VERBOSE` | 詳細出力 | 0 |
| `TEST_TIMEOUT` | テストタイムアウト（秒） | 10 |
| `MOCK_SERVER_URL` | MockServerのURL | http://localhost:1080 |

## テスト結果の期待値

正常に全テストが実行された場合：

| カテゴリ | 成功数 | 総数 | 成功率 |
|----------|--------|------|--------|
| Utils | 33 | 33 | 100% |
| Auth | 6 | 6 | 100% |
| Cookie | 9 | 9 | 100% |
| RequestValidator | 18 | 18 | 100% |
| Integration | 6 | 6 | 100% |
| E2E | 4 | 4 | 100% |
| **合計** | **76** | **76** | **100%** |

## CI/CD統合

GitHub Actionsでのテスト実行：

```yaml
# .github/workflows/test.yml
- name: Run tests
  run: |
    pio test -e native
    pio test -e native_integration
```

## その他のリソース

- [実装ガイド](implementation_guide.md) - 新機能追加時の参考
- [移行ロードマップ](migration_roadmap.md) - 将来の計画
- [プロジェクトREADME](../README.md) - プロジェクト概要