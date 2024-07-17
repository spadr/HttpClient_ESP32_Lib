// NetworkLayer.h
#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <WiFiClientSecure.h>
#include <algorithm>

namespace http
{

    class INetworkLayer
    {
    public:
        virtual ~INetworkLayer() = default;
        virtual bool connect(const std::string &host, int port) = 0;
        virtual void disconnect() = 0;
        virtual bool isConnected() const = 0;
        virtual size_t write(const uint8_t *buf, size_t size) = 0;
        virtual int read(uint8_t *buf, size_t size) = 0;
        virtual void setTimeouts(int connectTimeout, int readTimeout, int writeTimeout) = 0;
        virtual std::string readLine() = 0;
        virtual std::string read(size_t size) = 0;
        virtual void setVerifySsl(bool verify) = 0;
        virtual void setCACert(const char *rootCA) = 0;
        virtual void setClientCert(const char *cert, const char *private_key) = 0;
    };

    class MyWiFiSecureClient : public WiFiClientSecure
    {
    public:
        int getLastError() const { return _lastError; }
    };

    class WiFiSecureClient : public INetworkLayer, public MyWiFiSecureClient
    {
    public:
        WiFiSecureClient();
        ~WiFiSecureClient() override;

        bool connect(const std::string &host, int port) override;
        void disconnect() override;
        bool isConnected() const override;
        size_t write(const uint8_t *buf, size_t size) override;
        int read(uint8_t *buf, size_t size) override;
        void setTimeouts(int connectTimeout, int readTimeout, int writeTimeout) override;
        std::string readLine() override;
        std::string read(size_t size) override;
        void setVerifySsl(bool verify) override;
        void setCACert(const char *rootCA) override;
        void setClientCert(const char *cert, const char *private_key) override;

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };

} // namespace http