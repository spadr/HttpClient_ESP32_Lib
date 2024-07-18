#pragma once
#include <string>
#include <memory>
#include <chrono>
#include <WiFiClientSecure.h>

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
    };

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

        bool isConnectionValid(const std::string &host, int port) const;
        bool checkTimeout(const std::chrono::steady_clock::time_point &start,
                          const std::chrono::milliseconds &timeout) const;
    };

} // namespace canaspad