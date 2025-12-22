# HttpClient ESP32 Library - Tools Directory

This directory contains various tools for development, testing, and validation of the HttpClient ESP32 library.

## Directory Structure

```
tools/
├── setup/           # Environment setup tools
└── validation/      # Code validation and verification tools
```

## Quick Start

```bash
# 1. Setup development environment
./tools/setup/setup_dev_env.sh

# 2. Run all tests
./scripts/run-tests.sh -l all

# 3. Validate code
python3 tools/validation/check_includes.py
```

## Available Tools

### Setup Tools (`tools/setup/`)

- **`setup_dev_env.sh`** - Complete development environment setup
  ```bash
  ./tools/setup/setup_dev_env.sh
  ```

- **`get-platformio.py`** - PlatformIO installer
  ```bash
  python3 tools/setup/get-platformio.py
  ```

### Validation Tools (`tools/validation/`)

- **`check_includes.py`** - Validate include statements
  ```bash
  python3 tools/validation/check_includes.py
  ```

- **`check_test_duplicates.py`** - Check for duplicate test definitions
  ```bash
  python3 tools/validation/check_test_duplicates.py
  ```

- **`validate_platformio.py`** - Validate PlatformIO installation
  ```bash
  python3 tools/validation/validate_platformio.py
  ```

- **`verify_build_fixes.py`** - Verify build fixes are working
  ```bash
  python3 tools/validation/verify_build_fixes.py
  ```

## Notes

- The main test runner script (`scripts/run-tests.sh`) remains in the `scripts/` directory for backward compatibility
- Docker-related files remain in the `docker/` directory
- For detailed testing documentation, see [docs/TOOLS_AND_TESTING.md](../docs/TOOLS_AND_TESTING.md)