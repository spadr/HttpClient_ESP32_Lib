#pragma once

#ifdef ARDUINO_ARCH_NATIVE

#include "Connection.h"
#include <string>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif

namespace canaspad {

class NativeSocketConnection : public Connection {
public:
    NativeSocketConnection();
    ~NativeSocketConnection() override;

    bool connect(const std::string &host, int port) override;
    void disconnect() override;
    bool isConnected() const override;
    size_t write(const uint8_t *buf, size_t size) override;
    int read(uint8_t *buf, size_t size) override;
    void setTimeouts(const std::chrono::milliseconds &connectTimeout,
                     const std::chrono::milliseconds &readTimeout,
                     const std::chrono::milliseconds &writeTimeout) override;
    std::string readLine() override;
    std::string read(size_t size) override;
    
    // SSL is not supported in NativeSocketConnection (use for HTTP testing)
    void setVerifySsl(bool verify) override {}
    void setCACert(const char *rootCA) override {}
    void setClientCert(const char *cert) override {}
    void setClientPrivateKey(const char *privateKey) override {}

    bool connected() const override;
    int available() override;
    int read() override;
    int setTimeout(uint32_t seconds) override;

private:
#ifdef _WIN32
    SOCKET m_socket;
    static bool s_wsaInitialized;
#else
    int m_socket = -1;
#endif
    bool m_connected = false;
    std::chrono::milliseconds m_connectTimeout{5000};
    std::chrono::milliseconds m_readTimeout{5000};
    std::chrono::milliseconds m_writeTimeout{5000};
};

} // namespace canaspad

#endif // ARDUINO_ARCH_NATIVE

