#!/usr/bin/env python3
"""
インクルード依存関係チェックスクリプト
C++ソースファイルのインクルード関係を検証
"""
import os
import re
from pathlib import Path

def extract_includes(filepath):
    """ファイルからインクルード文を抽出"""
    includes = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                # #include "..." または #include <...> を検出
                match = re.match(r'#include\s*[<"]([^>"]+)[>"]', line)
                if match:
                    includes.append((line_num, match.group(1), line))
    except Exception as e:
        print(f"  ❌ ファイル読み込みエラー: {e}")
    return includes

def check_include_exists(include_path, source_file_dir):
    """インクルードファイルが存在するかチェック"""
    # 相対パスの場合、ソースファイルのディレクトリから検索
    if include_path.startswith('../') or not include_path.startswith('/'):
        full_path = os.path.join(source_file_dir, include_path)
        if os.path.exists(full_path):
            return True, full_path
        
        # src/ ディレクトリからも検索
        src_path = os.path.join('src', include_path)
        if os.path.exists(src_path):
            return True, src_path
    
    # システムヘッダー（通常存在するもの）
    system_headers = [
        'string', 'vector', 'memory', 'iostream', 'sstream', 'fstream',
        'chrono', 'thread', 'mutex', 'atomic', 'functional', 'algorithm',
        'unordered_map', 'map', 'set', 'unordered_set', 'optional',
        'Arduino.h', 'WiFi.h', 'time.h', 'cstring', 'cstdlib', 'cctype',
        'iomanip', 'curl/curl.h', 'unity.h', 'random', 'ctime'
    ]
    
    if any(include_path.endswith(header) or include_path == header for header in system_headers):
        return True, f"(system: {include_path})"
    
    return False, None

def analyze_source_file(filepath):
    """単一ソースファイルの分析"""
    print(f"\n📄 {filepath}")
    
    if not os.path.exists(filepath):
        print(f"  ❌ ファイルが存在しません")
        return False
    
    source_dir = os.path.dirname(filepath)
    includes = extract_includes(filepath)
    
    if not includes:
        print(f"  ℹ️  インクルード文なし")
        return True
    
    all_ok = True
    for line_num, include_path, full_line in includes:
        exists, resolved_path = check_include_exists(include_path, source_dir)
        if exists:
            if resolved_path.startswith('(system:'):
                print(f"  ✅ L{line_num:3d}: {include_path} {resolved_path}")
            else:
                print(f"  ✅ L{line_num:3d}: {include_path} → {resolved_path}")
        else:
            print(f"  ❌ L{line_num:3d}: {include_path} (見つかりません)")
            print(f"       {full_line}")
            all_ok = False
    
    return all_ok

def main():
    print("🔍 インクルード依存関係チェック")
    print("=" * 50)
    
    # チェック対象ファイル
    source_files = [
        "src/HttpClient.h",
        "src/core/HttpClient.cpp",
        "src/core/Request.cpp",
        "src/core/Response.cpp", 
        "src/core/RequestValidator.cpp",
        "src/utils/Utils.cpp",
        "src/auth/Auth.cpp",
        "src/cookie/CookieJar.cpp",
        "src/main.cpp"
    ]
    
    all_files_ok = True
    for source_file in source_files:
        if not analyze_source_file(source_file):
            all_files_ok = False
    
    print("\n" + "=" * 50)
    if all_files_ok:
        print("🎉 全てのインクルード依存関係: OK")
        print("\n📝 PlatformIOでビルド可能と推定されます")
        print("次のコマンドでビルドテストを実行:")
        print("  pio run -e m5stack-atom")
        return 0
    else:
        print("❌ インクルード依存関係に問題があります")
        print("修正が必要です")
        return 1

if __name__ == "__main__":
    exit(main())