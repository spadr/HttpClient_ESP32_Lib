#pragma once

#include <string>
#include <memory>
#include <chrono>

namespace canaspad
{

    class Connection
    {
    public:
        virtual ~Connection() = default;
        virtual bool connect(const std::string &host, int port) = 0;
        virtual void disconnect() = 0;
        virtual bool isConnected() const = 0;
        virtual size_t write(const uint8_t *buf, size_t size) = 0;
        virtual int read(uint8_t *buf, size_t size) = 0;
        virtual void setTimeouts(const std::chrono::milliseconds &connectTimeout,
                                 const std::chrono::milliseconds &readTimeout,
                                 const std::chrono::milliseconds &writeTimeout) = 0;
        virtual std::string readLine() = 0;
        virtual std::string read(size_t size) = 0;
        virtual void setVerifySsl(bool verify) = 0;
        virtual void setCACert(const char *rootCA) = 0;
        virtual void setClientCert(const char *cert) = 0;
        virtual void setClientPrivateKey(const char *privateKey) = 0;

        virtual bool connected() const = 0;
        virtual int available() = 0;
        virtual int read() = 0;
        virtual int setTimeout(uint32_t seconds) = 0;
    };

} // namespace canaspad