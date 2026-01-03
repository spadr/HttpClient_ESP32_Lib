#!/usr/bin/env python3
"""
テストファイル重複宣言チェックスクリプト
"""
import os
import glob
import re

def check_duplicate_declarations(filepath):
    """単一ファイル内の重複宣言をチェック"""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        
        # 変数宣言を追跡
        declarations = {}
        issues = []
        
        for line_num, line in enumerate(lines, 1):
            # std::string 変数宣言を検出
            match = re.search(r'std::string\s+(\w+)\s*=', line.strip())
            if match:
                var_name = match.group(1)
                if var_name in declarations:
                    issues.append({
                        'var_name': var_name,
                        'first_line': declarations[var_name],
                        'duplicate_line': line_num,
                        'content': line.strip()
                    })
                else:
                    declarations[var_name] = line_num
        
        return issues
        
    except Exception as e:
        print(f"  ❌ ファイル読み込みエラー: {e}")
        return None

def main():
    print("🔍 テストファイル重複宣言チェック")
    print("=" * 50)
    
    # テストファイルを検索
    test_files = glob.glob("test/**/*.cpp", recursive=True)
    test_files.sort()
    
    all_ok = True
    total_issues = 0
    
    for test_file in test_files:
        print(f"\n📄 {test_file}")
        
        issues = check_duplicate_declarations(test_file)
        if issues is None:
            continue
            
        if not issues:
            print("  ✅ 重複宣言なし")
        else:
            print(f"  ❌ 重複宣言 {len(issues)}件:")
            for issue in issues:
                print(f"    変数 '{issue['var_name']}' 重複:")
                print(f"      L{issue['first_line']}: 最初の宣言")
                print(f"      L{issue['duplicate_line']}: {issue['content']}")
            all_ok = False
            total_issues += len(issues)
    
    print("\n" + "=" * 50)
    if all_ok:
        print("🎉 全てのテストファイル: 重複宣言なし")
    else:
        print(f"❌ 重複宣言が {total_issues}件見つかりました")
    
    return 0 if all_ok else 1

if __name__ == "__main__":
    exit(main())