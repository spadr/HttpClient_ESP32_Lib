#!/usr/bin/env python3
"""
PlatformIO設定検証スクリプト
platformio.iniファイルの基本的な構文とライブラリ依存関係をチェック
"""
import os
import sys
import configparser

def validate_platformio_ini():
    """platformio.iniファイルの検証"""
    ini_path = "platformio.ini"
    
    if not os.path.exists(ini_path):
        print("❌ platformio.iniが見つかりません")
        return False
    
    try:
        config = configparser.ConfigParser()
        config.read(ini_path)
        
        print("✅ platformio.ini構文チェック: OK")
        
        # 環境定義の確認
        environments = [section for section in config.sections() if section.startswith('env:')]
        print(f"📋 検出された環境: {', '.join(environments)}")
        
        # 各環境の基本設定確認
        for env in environments:
            print(f"\n🔍 環境 [{env}] の確認:")
            
            # platformの確認
            if config.has_option(env, 'platform'):
                platform = config.get(env, 'platform')
                print(f"  Platform: {platform}")
            else:
                print(f"  ⚠️  Platform指定なし")
            
            # frameworkの確認
            if config.has_option(env, 'framework'):
                framework = config.get(env, 'framework')
                print(f"  Framework: {framework}")
            
            # build_flagsの確認
            if config.has_option(env, 'build_flags'):
                build_flags = config.get(env, 'build_flags')
                if '-std=gnu++17' in build_flags or '-std=gnu++2a' in build_flags:
                    print(f"  ✅ C++17/20サポート確認")
                else:
                    print(f"  ⚠️  C++標準指定なし")
            
            # lib_depsの確認
            if config.has_option(env, 'lib_deps'):
                lib_deps = config.get(env, 'lib_deps')
                deps = [dep.strip() for dep in lib_deps.split('\n') if dep.strip()]
                print(f"  ライブラリ依存: {len(deps)}個")
                for dep in deps:
                    print(f"    - {dep}")
        
        return True
        
    except configparser.Error as e:
        print(f"❌ platformio.ini構文エラー: {e}")
        return False

def check_source_files():
    """ソースファイルの基本チェック"""
    print("\n🔍 ソースファイルの確認:")
    
    required_files = [
        "src/HttpClient.h",
        "src/core/HttpClient.cpp",
        "src/core/Request.h",
        "src/core/Response.h",
        "src/Result.h"
    ]
    
    all_exist = True
    for file_path in required_files:
        if os.path.exists(file_path):
            print(f"  ✅ {file_path}")
        else:
            print(f"  ❌ {file_path} - 見つかりません")
            all_exist = False
    
    return all_exist

def check_test_structure():
    """テスト構造の確認"""
    print("\n🧪 テスト構造の確認:")
    
    test_dirs = [
        "test/unit",
        "test/integration", 
        "test/e2e",
        "test/helpers"
    ]
    
    for test_dir in test_dirs:
        if os.path.exists(test_dir):
            files = os.listdir(test_dir)
            cpp_files = [f for f in files if f.endswith('.cpp') or f.endswith('.h')]
            print(f"  ✅ {test_dir} ({len(cpp_files)}ファイル)")
        else:
            print(f"  ⚠️  {test_dir} - ディレクトリなし")

def main():
    print("🚀 PlatformIO設定検証ツール")
    print("=" * 40)
    
    # 作業ディレクトリの確認
    if not os.path.exists("platformio.ini"):
        print("❌ プロジェクトルートで実行してください")
        sys.exit(1)
    
    # 各種チェック実行
    ini_valid = validate_platformio_ini()
    files_valid = check_source_files()
    check_test_structure()
    
    print("\n" + "=" * 40)
    if ini_valid and files_valid:
        print("🎉 PlatformIO設定検証: 成功")
        print("\n📝 次のステップ:")
        print("1. PlatformIOのインストール: pip install platformio")
        print("2. ビルドテスト: pio run -e m5stack-atom")
        print("3. ユニットテスト: pio test -e native")
        return 0
    else:
        print("❌ PlatformIO設定検証: 問題あり")
        return 1

if __name__ == "__main__":
    sys.exit(main())