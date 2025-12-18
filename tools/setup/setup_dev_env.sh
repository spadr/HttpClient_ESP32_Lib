#!/bin/bash

# HttpClient_ESP32_Lib 開発環境セットアップスクリプト
# 作成日: 2025-01-21

set -e

echo "🔧 HttpClient_ESP32_Lib 開発環境セットアップを開始します..."

# OS検出
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="mac"
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
    OS="windows"
else
    echo "❌ サポートされていないOS: $OSTYPE"
    exit 1
fi

echo "📍 検出されたOS: $OS"

# 1. 必要なパッケージのインストール
echo "📦 必要なパッケージをインストールしています..."

if [ "$OS" = "linux" ]; then
    # Linuxの場合
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y g++ gcc make python3 python3-pip docker.io docker-compose
    elif command -v yum &> /dev/null; then
        sudo yum install -y gcc-c++ gcc make python3 python3-pip docker docker-compose
    else
        echo "⚠️ パッケージマネージャが見つかりません。手動でインストールしてください。"
    fi
elif [ "$OS" = "mac" ]; then
    # macOSの場合
    if ! command -v brew &> /dev/null; then
        echo "📦 Homebrewをインストールしています..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    brew install gcc python3 docker docker-compose
fi

# 2. PlatformIOのインストール
echo "🚀 PlatformIOをインストールしています..."
if ! command -v pio &> /dev/null; then
    python3 -m pip install --user platformio
    export PATH=$PATH:~/.local/bin
else
    echo "✅ PlatformIOは既にインストールされています"
fi

# 3. PlatformIOの依存関係インストール
echo "📚 PlatformIOの依存関係をインストールしています..."
pio pkg install

# 4. Dockerサービスの確認
echo "🐳 Dockerサービスを確認しています..."
if [ "$OS" = "linux" ]; then
    if ! systemctl is-active --quiet docker; then
        sudo systemctl start docker
        sudo systemctl enable docker
    fi
    # 現在のユーザーをdockerグループに追加
    if ! groups $USER | grep -q docker; then
        sudo usermod -aG docker $USER
        echo "⚠️ dockerグループに追加されました。再ログインが必要です。"
    fi
fi

# 5. テスト環境の準備
echo "🧪 テスト環境を準備しています..."
if [ -f "docker/test-services.yml" ]; then
    cd docker
    docker-compose -f test-services.yml pull
    cd ..
fi

# 6. 初回テストの実行
echo "🔍 セットアップの確認..."
echo "コンパイラ確認:"
g++ --version || echo "❌ g++が見つかりません"
echo ""
echo "PlatformIO確認:"
pio --version || echo "❌ PlatformIOが見つかりません"
echo ""
echo "Docker確認:"
docker --version || echo "❌ Dockerが見つかりません"

echo ""
echo "✅ セットアップが完了しました！"
echo ""
echo "📝 次のステップ:"
echo "1. Cookieテストのコンパイルと実行:"
echo "   g++ -std=c++17 -DNATIVE_TEST -DARDUINO_ARCH_NATIVE -I src -I test/helpers \\"
echo "     test/unit/cookie/test_cookie.cpp src/cookie/CookieJar.cpp src/utils/Utils.cpp \\"
echo "     src/utils/HttpMethod.cpp src/native_arduino_compat.cpp -o test_cookie && ./test_cookie"
echo ""
echo "2. 全テストの実行:"
echo "   ./scripts/run-tests.sh -l all"
echo ""
echo "3. PlatformIOでのテスト:"
echo "   pio test -e native"
echo ""
echo "⚠️ 注意: Dockerグループに追加された場合は、一度ログアウト・ログインしてください。"