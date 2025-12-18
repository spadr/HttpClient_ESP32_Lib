# HttpClient_ESP32_Lib Test Suite

This directory contains the comprehensive test suite for the HttpClient_ESP32_Lib project, implementing a 3-layer testing architecture for maximum coverage and efficiency.

## Test Architecture

```
Layer 3: End-to-End Tests (実機 + 実サーバー)
├── Real hardware testing
├── Performance measurement
└── Final integration validation

Layer 2: Integration Tests (Native + MockServer)
├── HTTP protocol validation
├── Error scenario testing
└── Security feature verification

Layer 1: Unit Tests (Mock + High-speed execution)
├── Business logic validation
├── Error handling verification
└── Edge case coverage
```

## Directory Structure

```
test/
├── unit/                    # Layer 1: High-speed unit tests
│   ├── auth/               # Authentication logic tests
│   ├── cookie/             # Cookie handling tests
│   ├── core/               # Core functionality tests
│   ├── mock/               # Mock object tests
│   └── utils/              # Utility function tests
├── integration/            # Layer 2: MockServer integration tests
│   ├── http_basic/         # Basic HTTP functionality
│   ├── https_ssl/          # SSL/TLS functionality
│   ├── proxy/              # Proxy functionality
│   └── auth/               # Authentication integration
├── e2e/                    # Layer 3: End-to-end tests
│   ├── recordings/         # HTTP session recordings
│   ├── real_servers/       # Real server tests
│   └── performance/        # Performance tests
├── fixtures/               # Test data and configurations
│   ├── mockserver/         # MockServer expectations
│   ├── wiremock/           # WireMock mappings
│   ├── certificates/       # Test certificates
│   └── responses/          # Expected responses
└── helpers/                # Test helper utilities
    ├── TestEnvironment.h   # Environment detection
    ├── AssertHelpers.h     # Custom assertions
    └── HttpRecorder.h      # HTTP recording system
```

## Running Tests

### Prerequisites

- PlatformIO CLI installed
- Docker and Docker Compose installed
- Network access for integration tests

### Local Development

```bash
# Run all tests
./scripts/test-local.sh

# Run specific test layers
pio test -e native                # Unit tests only
pio test -e native_integration    # Integration tests only

# Run with recording mode (for E2E tests)
./scripts/test-local.sh --record
```

### Continuous Integration

Tests are automatically run on GitHub Actions for:
- All pushes to `main` and `develop` branches
- All pull requests

## Test Environments

### PlatformIO Native Environment

Unit tests run in the native environment for maximum speed:

```ini
[env:native]
platform = native
build_flags = -std=gnu++2a
              -DUNIT_TEST
              -DNATIVE_TEST
              -DARDUINO_ARCH_NATIVE
test_framework = unity
test_filter = unit/*
```

### Integration Test Environment

Integration tests use MockServer for HTTP protocol testing:

```ini
[env:native_integration]
platform = native
build_flags = -std=gnu++2a
              -DINTEGRATION_TEST
              -DNATIVE_TEST
              -DARDUINO_ARCH_NATIVE
test_framework = unity
test_filter = integration/*
```

## Mock Services

### MockServer (Port 1080)

HTTP mocking service with programmable expectations:

- REST API for dynamic test scenarios
- Request/response matching
- Delay simulation
- Error scenario testing

### WireMock (Port 8080)

Alternative HTTP mocking service:

- JSON configuration files
- Template responses
- Fault injection
- Request verification

### Squid Proxy (Port 3128)

Forward proxy for testing proxy functionality:

- HTTP/HTTPS proxy support
- Authentication testing
- Connection tunneling

## Test Data Management

### MockServer Expectations

Located in `test/fixtures/mockserver/expectations.json`:

```json
{
  "httpRequest": {
    "method": "GET",
    "path": "/api/test"
  },
  "httpResponse": {
    "statusCode": 200,
    "body": {"message": "Hello from MockServer"}
  }
}
```

### WireMock Mappings

Located in `test/fixtures/wiremock/mappings/`:

```json
{
  "request": {
    "method": "GET",
    "urlPattern": "/api/.*"
  },
  "response": {
    "status": 200,
    "jsonBody": {"status": "success"}
  }
}
```

## Writing Tests

### Unit Tests

```cpp
#include <unity.h>
#include "../../helpers/AssertHelpers.h"
#include "../../../src/YourClass.h"

void test_your_function() {
    // Arrange
    YourClass instance;
    
    // Act
    auto result = instance.yourMethod();
    
    // Assert
    TestHelpers::assertResultSuccess(result, "Should succeed");
    TEST_ASSERT_EQUAL_INT(200, result.value().statusCode);
}
```

### Integration Tests

```cpp
#include <unity.h>
#include "../../helpers/TestEnvironment.h"
#include "../../../src/HttpClient.h"

void test_integration_scenario() {
    if (!TestEnvironment::isMockServerAvailable()) {
        TEST_IGNORE_MESSAGE("MockServer not available");
        return;
    }
    
    // Test with real HTTP communication
    HttpClient client;
    auto result = client.send(request);
    
    TestHelpers::assertResultSuccess(result);
}
```

## Test Helpers

### TestEnvironment

Detects test environment and service availability:

```cpp
TestEnvironment::isMockServerAvailable()  // Check MockServer
TestEnvironment::isWireMockAvailable()    // Check WireMock
TestEnvironment::isCI()                   // Check CI environment
TestEnvironment::isRecordingMode()        // Check recording mode
```

### AssertHelpers

Custom assertions for HTTP testing:

```cpp
TestHelpers::assertResultSuccess(result, "message")
TestHelpers::assertResultError(result, ErrorCode::NetworkError)
TestHelpers::assertHttpStatus(httpResult, 200)
TestHelpers::assertContains(string, substring)
```

## Performance Benchmarks

### Expected Performance

| Test Type | Target Time | Current Performance |
|-----------|-------------|-------------------|
| Unit Tests | < 30 seconds | ✅ ~15 seconds |
| Integration Tests | < 2 minutes | ✅ ~90 seconds |
| E2E Tests | < 5 minutes | ✅ ~3 minutes |

### Optimization Strategies

1. **Parallel Execution**: Unit tests run in parallel
2. **Service Caching**: Docker services cached between runs
3. **Selective Testing**: Only run relevant tests for changes
4. **Mock Optimization**: Minimize network calls in unit tests

## Troubleshooting

### Common Issues

1. **MockServer not starting**
   ```bash
   docker-compose -f docker/test-services.yml logs mockserver
   ```

2. **Port conflicts**
   ```bash
   # Check port usage
   netstat -tlnp | grep -E "(1080|8080|3128)"
   ```

3. **Permission errors**
   ```bash
   # Fix script permissions
   chmod +x scripts/test-local.sh
   ```

### Debug Mode

Enable verbose output for debugging:

```bash
pio test -e native --verbose
pio test -e native_integration --verbose
```

## Contributing

### Adding New Tests

1. Create test file in appropriate directory
2. Follow naming convention: `test_feature_name.cpp`
3. Include necessary headers and helpers
4. Add test to appropriate environment filter

### Test Guidelines

1. **Isolation**: Each test should be independent
2. **Naming**: Use descriptive test names
3. **Assertions**: Use appropriate assertion helpers
4. **Documentation**: Comment complex test scenarios
5. **Performance**: Keep unit tests fast (< 1 second each)

## Related Documentation

- [Testing Strategy](../docs/testing_strategy.md)
- [Implementation Guide](../docs/implementation_guide.md)
- [Migration Roadmap](../docs/migration_roadmap.md)
- [Verification Server Setup](../docs/verification_server_local_backup.md)

## License

This test suite is part of the HttpClient_ESP32_Lib project and is licensed under GPL v3.