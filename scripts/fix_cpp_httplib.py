import os
import shutil
import sys
from os.path import abspath, dirname, exists, isdir, join

KEEP_FILES = {"httplib.h", "LICENSE", "README.md", "library.json", ".piopm"}


def cleanup_cpp_httplib_lib(lib_dir):
    """cpp-httplib はヘッダオンリー。サブディレクトリと .cpp はビルド不要。"""
    if not isdir(lib_dir):
        return False

    for item in os.listdir(lib_dir):
        if item in KEEP_FILES or item.startswith("."):
            continue

        path = join(lib_dir, item)
        try:
            if isdir(path):
                print(f"Removing unnecessary directory: {path}")
                shutil.rmtree(path)
            elif item.endswith(".cpp"):
                print(f"Removing unnecessary source file: {path}")
                os.remove(path)
        except Exception as e:
            print(f"Warning: Failed to remove {path}: {e}")

    return True


def find_cpp_httplib_dirs(libdeps_dir):
    if not isdir(libdeps_dir):
        return []

    return [
        join(libdeps_dir, item)
        for item in os.listdir(libdeps_dir)
        if "cpp-httplib" in item and isdir(join(libdeps_dir, item))
    ]


def cleanup_for_env(project_dir, pioenv):
    libdeps_dir = join(project_dir, ".pio", "libdeps", pioenv)
    print(f"Checking cpp-httplib cleanup in {libdeps_dir}...")

    lib_dirs = find_cpp_httplib_dirs(libdeps_dir)
    if not lib_dirs:
        print("cpp-httplib directory not found (yet).")
        return False

    for lib_dir in lib_dirs:
        cleanup_cpp_httplib_lib(lib_dir)

    return True


def register_platformio_hooks(env):
    project_dir = env.subst("$PROJECT_DIR")
    pioenv = env.subst("$PIOENV")

    def cleanup_cpp_httplib(source, target, env):
        cleanup_for_env(project_dir, pioenv)

    env.AddPreAction("buildprog", cleanup_cpp_httplib)

    for lib in env.GetLibBuilders():
        lib_name = lib.get_name()
        if "cpp-httplib" not in lib_name and "httplib" not in lib_name:
            continue
        for item in lib.get_build_items():
            env.AddPreAction(item, cleanup_cpp_httplib)

    cleanup_for_env(project_dir, pioenv)


try:
    Import("env")

    register_platformio_hooks(env)
except Exception:
    pass


if __name__ == "__main__":
    project_dir = dirname(dirname(abspath(__file__)))
    env_name = sys.argv[1] if len(sys.argv) > 1 else "test_unit"
    cleanup_for_env(project_dir, env_name)
