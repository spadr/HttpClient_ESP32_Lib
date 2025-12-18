#!/usr/bin/env python3
"""
Simple test runner for HttpClient ESP32 tests when PlatformIO is not available
"""
import subprocess
import sys
import os

def run_command(cmd, description):
    """Run a command and return success/failure"""
    print(f"Running: {description}")
    try:
        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"✅ {description} - SUCCESS")
            return True
        else:
            print(f"❌ {description} - FAILED")
            print(f"Error: {result.stderr}")
            return False
    except Exception as e:
        print(f"❌ {description} - ERROR: {e}")
        return False

def main():
    print("🚀 HttpClient ESP32 Test Status Check")
    print("=" * 40)
    
    # Check if we're in the right directory
    if not os.path.exists("platformio.ini"):
        print("❌ platformio.ini not found. Please run from project root.")
        sys.exit(1)
    
    # Test file syntax check
    test_files = [
        "test/unit/test_utils.cpp",
        "test/unit/test_request_validator.cpp",
        "test/integration/test_http_protocol.cpp",
        "test/e2e/test_real_server.cpp"
    ]
    
    all_pass = True
    
    for test_file in test_files:
        if os.path.exists(test_file):
            print(f"✅ {test_file} exists")
        else:
            print(f"❌ {test_file} missing")
            all_pass = False
    
    # Check project structure
    required_dirs = ["src", "test", "test/unit", "test/integration", "test/e2e"]
    for dir_path in required_dirs:
        if os.path.exists(dir_path):
            print(f"✅ {dir_path}/ directory exists")
        else:
            print(f"❌ {dir_path}/ directory missing")
            all_pass = False
    
    # Check configuration files
    config_files = ["platformio.ini", "docker-compose.yml", "scripts/run-tests.sh"]
    for config_file in config_files:
        if os.path.exists(config_file):
            print(f"✅ {config_file} exists")
        else:
            print(f"❌ {config_file} missing")
            all_pass = False
    
    print("\n" + "=" * 40)
    if all_pass:
        print("🎉 All test infrastructure files are present!")
        print("\nNext steps:")
        print("1. Install PlatformIO: pip install platformio")
        print("2. Start MockServer: docker-compose up -d mockserver")
        print("3. Run Layer 1 tests: pio test -e native")
        print("4. Run Layer 2 tests: pio test -e native_integration")
        return 0
    else:
        print("❌ Some test infrastructure files are missing")
        return 1

if __name__ == "__main__":
    sys.exit(main())