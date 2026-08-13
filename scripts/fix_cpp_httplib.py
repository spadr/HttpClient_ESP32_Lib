import os
import shutil
import sys
from os.path import abspath, dirname, exists, isdir, isfile, join

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


def prepare_cpp_httplib(project_dir, pioenv):
    libdeps_dir = join(project_dir, ".pio", "libdeps", pioenv)
    print(f"Checking cpp-httplib cleanup in {libdeps_dir}...")

    lib_dirs = find_cpp_httplib_dirs(libdeps_dir)
    if not lib_dirs:
        print("cpp-httplib directory not found (yet).")
        return False

    for lib_dir in lib_dirs:
        cleanup_cpp_httplib_lib(lib_dir)

    return True


def deploy_unity_config(project_dir, pioenv):
    """Unity ライブラリ単体ビルド時に unity_config.h を参照できるよう配置する。"""
    libdeps_dir = join(project_dir, ".pio", "libdeps", pioenv)
    config_src = join(project_dir, "test", "unity_config.h")
    print(f"Deploying unity_config.h for {pioenv}...")

    if not isfile(config_src):
        print(f"unity_config.h not found at {config_src}")
        return False

    if not isdir(libdeps_dir):
        print(f"libdeps directory not found at {libdeps_dir}")
        return False

    deployed = False
    for item in os.listdir(libdeps_dir):
        if not item.startswith("Unity"):
            continue

        unity_src_dir = join(libdeps_dir, item, "src")
        if not isdir(unity_src_dir):
            continue

        dst = join(unity_src_dir, "unity_config.h")
        shutil.copy2(config_src, dst)
        print(f"Deployed unity_config.h to {dst}")
        deployed = True

    if not deployed:
        print("Unity library not found (yet).")

    return deployed


def prepare_test_deps(project_dir, pioenv):
    prepare_cpp_httplib(project_dir, pioenv)
    deploy_unity_config(project_dir, pioenv)


def configure_native_compiler(env):
    """PlatformIO native uses `cc`/`c++`; Windows MinGW often only ships gcc/g++."""
    use_gcc = shutil.which("gcc")
    use_gxx = shutil.which("g++")
    cc_path = shutil.which("cc")
    cxx_path = shutil.which("c++")

    # A copied gcc.exe named cc.exe cannot find cc1; prefer real gcc/g++ on Windows.
    if sys.platform.startswith("win") and use_gcc:
        print("Using gcc as CC for native tests")
        env.Replace(CC="gcc")
    elif cc_path is None and use_gcc:
        print("cc not found; using gcc")
        env.Replace(CC="gcc")

    if sys.platform.startswith("win") and use_gxx:
        print("Using g++ as CXX/LINK for native tests")
        env.Replace(CXX="g++", LINK="g++")
        env.Append(LIBS=["ws2_32"])
    elif cxx_path is None and use_gxx:
        print("c++ not found; using g++")
        env.Replace(CXX="g++", LINK="g++")


def register_platformio_hooks(env):
    project_dir = env.subst("$PROJECT_DIR")
    pioenv = env.subst("$PIOENV")
    configure_native_compiler(env)

    def on_prepare(source, target, env):
        prepare_test_deps(project_dir, pioenv)

    env.AddPreAction("buildprog", on_prepare)

    for lib in env.GetLibBuilders():
        lib_name = lib.get_name()
        if "cpp-httplib" not in lib_name and "httplib" not in lib_name and not lib_name.startswith("Unity"):
            continue
        for item in lib.get_build_items():
            env.AddPreAction(item, on_prepare)

    prepare_test_deps(project_dir, pioenv)


try:
    Import("env")

    register_platformio_hooks(env)
except Exception:
    pass


if __name__ == "__main__":
    project_dir = dirname(dirname(abspath(__file__)))
    env_name = sys.argv[1] if len(sys.argv) > 1 else "test_unit"
    prepare_test_deps(project_dir, env_name)
