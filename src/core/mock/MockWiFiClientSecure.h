#pragma once

#include "../Connection.h"
#include "CommunicationLog.h"
#include "../CommonTypes.h"

#include <vector>
#include <queue>
#include <string>
#include <chrono>

namespace canaspad
{

    class MockWiFiClientSecure : public Connection
    {
    private:
        bool m_connected;
        CommunicationLog m_log;
        std::queue<std::vector<uint8_t>> m_responseQueue;
        std::vector<uint8_t> m_receiveBuffer;
        std::chrono::steady_clock::time_point m_operationStart;
        std::chrono::milliseconds m_readTimeout{0};
        bool m_simulateTimeout{false};

        // SSL 関連の設定を保持する変数
        bool m_verifySsl;
        std::string m_caCert;
        std::string m_clientCert;
        std::string m_clientPrivateKey;

    public:
        MockWiFiClientSecure(const ClientOptions &options);

        bool connect(const std::string &host, int port) override;
        void disconnect() override;
        bool isConnected() const override;
        size_t write(const uint8_t *buf, size_t size) override;
        int read(uint8_t *buf, size_t size) override;
        int setTimeout(uint32_t seconds) override;
        void setTimeouts(const std::chrono::milliseconds &connectTimeout,
                         const std::chrono::milliseconds &readTimeout,
                         const std::chrono::milliseconds &writeTimeout) override;
        std::string readLine() override;
        std::string read(size_t size) override;
        void setVerifySsl(bool verify) override;
        void setCACert(const char *rootCA) override;
        void setClientCert(const char *cert) override;
        void setClientPrivateKey(const char *privateKey) override;
        bool connected() const override;
        int available() override;
        int read() override;

        // テスト用メソッド
        void injectResponse(const std::vector<uint8_t> &response);
        const CommunicationLog &getCommunicationLog() const;
        void simulateTimeout(bool simulate);
        void setOptions(const ClientOptions &options);

        // SSL 設定を確認するためのGetter メソッド
        bool getVerifySsl() const;
        const std::string &getCACert() const;
        const std::string &getClientCert() const;
        const std::string &getClientPrivateKey() const;
    };

} // namespace canaspad