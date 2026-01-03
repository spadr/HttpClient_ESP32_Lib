import os
import shutil
from os.path import exists, isdir, join

Import("env")

# ライブラリの依存関係ディレクトリを取得
# 通常は .pio/libdeps/env_name/
project_libdeps_dir = join(
    env.subst("$PROJECT_DIR"), ".pio", "libdeps", env.subst("$PIOENV")
)


def cleanup_cpp_httplib(source, target, env):
    """
    cpp-httplibの不要なディレクトリ（docker, benchmark, exampleなど）を削除する。
    これらはヘッダオンリーライブラリの使用には不要であり、
    かつPlatformIOが勝手にビルドしようとしてエラーを引き起こすため。
    """
    print(f"Checking cpp-httplib cleanup in {project_libdeps_dir}...")

    # 複数のバージョンのライブラリがある可能性を考慮して探索するか、
    # あるいは 'cpp-httplib' という名前を含むディレクトリを探す

    # PlatformIOがリネームしている可能性があるが、通常はリポジトリ名やライブラリ名になる
    # github URL指定の場合は 'cpp-httplib' になることが多い

    target_lib_dir = None

    # cpp-httplibディレクトリを探す
    if isdir(project_libdeps_dir):
        for item in os.listdir(project_libdeps_dir):
            if "cpp-httplib" in item:
                target_lib_dir = join(project_libdeps_dir, item)
                break

    if target_lib_dir and isdir(target_lib_dir):
        # 削除対象のディレクトリリスト
        dirs_to_remove = ["docker", "benchmark", "example", "test"]

        for d in dirs_to_remove:
            dir_path = join(target_lib_dir, d)
            if exists(dir_path) and isdir(dir_path):
                try:
                    print(f"Removing unnecessary directory: {dir_path}")
                    shutil.rmtree(dir_path)
                except Exception as e:
                    print(f"Warning: Failed to remove {dir_path}: {e}")
    else:
        print(
            "cpp-httplib directory not found (yet). It might be installed in the next step."
        )


# 'pre' スクリプトとして実行されるため、ライブラリのインストール前かもしれない。
# そのため、pre: ではなく、本来はライブラリインストール後に走らせたいが、
# PlatformIOのフックタイミング的に 'pre' でも2回目以降のビルドでは有効。
# 初回ビルドで失敗する可能性を下げるため、ライブラリインストールステップへのフックが理想だが、
# 単純化のために 'pre' で実行し、もしディレクトリがなければ何もしない（初回はエラーになるかもしれないが2回目で通る）。
# あるいは、env.AddPreAction("compile", cleanup_cpp_httplib) のようにコンパイル前にフックする。

# コンパイルアクションの前に実行するように登録
env.AddPreAction("checkprogsize", cleanup_cpp_httplib)  # ダミーアクションへのフック
env.AddPreAction("buildprog", cleanup_cpp_httplib)

# すべてのソースファイルのコンパイル前に実行
for src in env.get("SRC_BUILD_TARGETS", []):
    env.AddPreAction(src, cleanup_cpp_httplib)

# プロジェクト初期化時にも一度実行しておく
cleanup_cpp_httplib(None, None, env)
