#!/usr/bin/env python3
"""
ビルドエラー修正確認スクリプト
報告されたビルドエラーが修正されているかチェック
"""
import os
import re

def check_main_cpp_issues():
    """main.cppの問題をチェック"""
    print("🔍 main.cpp の問題チェック:")
    
    try:
        with open("src/main.cpp", "r", encoding="utf-8") as f:
            content = f.read()
            
        # resultHttps の未定義変数チェック
        if "resultHttps" in content:
            print("  ❌ 未定義変数 'resultHttps' がまだ存在します")
            return False
        else:
            print("  ✅ 未定義変数 'resultHttps' は修正済み")
            
        # result 変数の使用確認
        if "result.isSuccess()" in content:
            print("  ✅ 'result' 変数が正しく使用されています")
        else:
            print("  ⚠️  'result' 変数の使用が見つかりません")
            
        return True
        
    except Exception as e:
        print(f"  ❌ ファイル読み込みエラー: {e}")
        return False

def check_httpclient_errors():
    """HttpClient.cppのエラーをチェック"""
    print("\n🔍 HttpClient.cpp の問題チェック:")
    
    try:
        with open("src/core/HttpClient.cpp", "r", encoding="utf-8") as f:
            content = f.read()
            
        # ConnectionFailed エラーコードチェック
        if "ErrorCode::ConnectionFailed" in content:
            print("  ❌ 未定義エラーコード 'ConnectionFailed' がまだ存在します")
            return False
        else:
            print("  ✅ 未定義エラーコード 'ConnectionFailed' は修正済み")
            
        # NetworkError の使用確認
        if "ErrorCode::NetworkError" in content:
            print("  ✅ 'NetworkError' が正しく使用されています")
        else:
            print("  ⚠️  代替エラーコードが見つかりません")
            
        # ostringstream::reserve の問題チェック
        if "oss.reserve(" in content:
            print("  ❌ 'oss.reserve()' がまだ存在します")
            return False
        else:
            print("  ✅ 'oss.reserve()' は削除済み")
            
        return True
        
    except Exception as e:
        print(f"  ❌ ファイル読み込みエラー: {e}")
        return False

def check_error_code_definitions():
    """ErrorCode定義をチェック"""
    print("\n🔍 ErrorCode 定義チェック:")
    
    try:
        with open("src/Result.h", "r", encoding="utf-8") as f:
            content = f.read()
            
        # 必要なエラーコードの存在確認
        required_codes = [
            "NetworkError",
            "InvalidResponse", 
            "UnsupportedProtocol",
            "InvalidURL"
        ]
        
        all_found = True
        for code in required_codes:
            if code in content:
                print(f"  ✅ ErrorCode::{code} 定義済み")
            else:
                print(f"  ❌ ErrorCode::{code} が見つかりません")
                all_found = False
                
        return all_found
        
    except Exception as e:
        print(f"  ❌ ファイル読み込みエラー: {e}")
        return False

def check_build_configuration():
    """ビルド設定をチェック"""
    print("\n🔍 ビルド設定チェック:")
    
    try:
        with open("platformio.ini", "r", encoding="utf-8") as f:
            content = f.read()
            
        # C++17設定の確認
        if "-std=gnu++17" in content:
            print("  ✅ C++17 設定確認")
        else:
            print("  ⚠️  C++17 設定が見つかりません")
            
        # デバッグレベル確認
        if "CORE_DEBUG_LEVEL=5" in content:
            print("  ✅ デバッグレベル設定確認")
        elif "CORE_DEBUG_LEVEL=10" in content:
            print("  ⚠️  デバッグレベルが高すぎる可能性があります")
        else:
            print("  ℹ️  デバッグレベル設定なし")
            
        return True
        
    except Exception as e:
        print(f"  ❌ ファイル読み込みエラー: {e}")
        return False

def main():
    print("🔧 ビルドエラー修正確認ツール")
    print("=" * 50)
    
    # 各種チェック実行
    main_ok = check_main_cpp_issues()
    httpclient_ok = check_httpclient_errors() 
    errorcode_ok = check_error_code_definitions()
    config_ok = check_build_configuration()
    
    print("\n" + "=" * 50)
    
    if main_ok and httpclient_ok and errorcode_ok and config_ok:
        print("🎉 全てのビルドエラーが修正されました！")
        print("\n📝 次のステップ:")
        print("1. PlatformIOでビルド再実行:")
        print("   pio run -e m5stack-atom")
        print("2. または VSCodeのビルドボタンをクリック")
        return 0
    else:
        print("❌ まだ修正が必要な問題があります")
        return 1

if __name__ == "__main__":
    exit(main())