#ifdef NATIVE_TEST

#include "WiFiSecureConnection.h"
#include <iostream>

namespace canaspad {

WiFiSecureConnection::WiFiSecureConnection() : _lastError(0) {
    // Mock implementation for native tests
}

WiFiSecureConnection::~WiFiSecureConnection() {
    // Mock implementation for native tests
}

bool WiFiSecureConnection::connect(const std::string& host, int port) {
    // Mock implementation for native tests
    m_connectedHost = host;
    m_connectedPort = port;
    return true;
}

bool WiFiSecureConnection::isConnected() const {
    // Mock implementation for native tests
    return false;
}

void WiFiSecureConnection::disconnect() {
    // Mock implementation for native tests
    m_connectedHost.clear();
    m_connectedPort = 0;
}

size_t WiFiSecureConnection::write(const uint8_t* data, size_t size) {
    // Mock implementation for native tests
    return size;
}

int WiFiSecureConnection::available() {
    // Mock implementation for native tests
    return 0;
}

int WiFiSecureConnection::read(uint8_t* buffer, size_t size) {
    // Mock implementation for native tests
    return 0;
}

int WiFiSecureConnection::read() {
    // Mock implementation for native tests
    return -1;
}

std::string WiFiSecureConnection::readLine() {
    // Mock implementation for native tests
    return "";
}

std::string WiFiSecureConnection::read(size_t size) {
    // Mock implementation for native tests
    return "";
}

void WiFiSecureConnection::setVerifySsl(bool verify) {
    // Mock implementation for native tests
    m_verifySsl = verify;
}

void WiFiSecureConnection::setCACert(const char* rootCA) {
    // Mock implementation for native tests
    if (rootCA) {
        m_caCert = rootCA;
    }
}

void WiFiSecureConnection::setClientCert(const char* cert) {
    // Mock implementation for native tests
    if (cert) {
        m_clientCert = cert;
    }
}

void WiFiSecureConnection::setClientPrivateKey(const char* private_key) {
    // Mock implementation for native tests
    if (private_key) {
        m_privateKey = private_key;
    }
}

void WiFiSecureConnection::setTimeouts(const std::chrono::milliseconds& connectTimeout,
                                      const std::chrono::milliseconds& readTimeout,
                                      const std::chrono::milliseconds& writeTimeout) {
    // Mock implementation for native tests
    m_connectTimeout = connectTimeout;
    m_readTimeout = readTimeout;
    m_writeTimeout = writeTimeout;
}

bool WiFiSecureConnection::connected() const {
    // Mock implementation for native tests
    return false;
}

int WiFiSecureConnection::setTimeout(uint32_t seconds) {
    // Mock implementation for native tests
    return 1;
}

bool WiFiSecureConnection::isConnectionValid(const std::string& host, int port) const {
    // Mock implementation for native tests
    return m_connectedHost == host && m_connectedPort == port;
}

bool WiFiSecureConnection::checkTimeout(const std::chrono::steady_clock::time_point& start,
                                       const std::chrono::milliseconds& timeout) const {
    // Mock implementation for native tests
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    return elapsed >= timeout;
}

} // namespace canaspad

#endif // NATIVE_TEST