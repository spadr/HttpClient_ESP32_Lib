#pragma once

#include "Connection.h"
#include <WiFiClientSecure.h>

namespace canaspad
{

    class WiFiSecureConnection : public Connection, public WiFiClientSecure
    {
    public:
        WiFiSecureConnection();
        ~WiFiSecureConnection() override;

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
        void setVerifySsl(bool verify) override;
        void setCACert(const char *rootCA) override;
        void setClientCert(const char *cert) override;
        void setClientPrivateKey(const char *private_key) override;

        int getLastError() const { return _lastError; }
        bool connected() const override;
        int setTimeout(uint32_t seconds) override;
        int available() override;
        int read() override;

    private:
        std::chrono::milliseconds m_connectTimeout{30000};
        std::chrono::milliseconds m_readTimeout{30000};
        std::chrono::milliseconds m_writeTimeout{30000};
        bool m_verifySsl = true;
        std::string m_caCert = "";
        std::string m_clientCert = "";
        std::string m_privateKey = "";

        std::chrono::steady_clock::time_point m_lastUsed;
        std::chrono::milliseconds m_keepAliveTimeout{30000}; // デフォルト値を設定
        std::string m_connectedHost;
        int m_connectedPort;
        int _lastError;

        bool isConnectionValid(const std::string &host, int port) const;
        bool checkTimeout(const std::chrono::steady_clock::time_point &start,
                          const std::chrono::milliseconds &timeout) const;
    };

} // namespace canaspad