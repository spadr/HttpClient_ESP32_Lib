#pragma once

#include "../Connection.h"
#include "CommunicationLog.h"
#include "ResponseInjector.h"

#include <vector>
#include <queue>
#include <string>
#include <memory>

namespace canaspad
{

    class MockWiFiClientSecure : public Connection
    {
    private:
        bool m_connected;
        CommunicationLog m_log;
        ResponseInjector m_injector;
        std::vector<uint8_t> m_receiveBuffer;

    public:
        MockWiFiClientSecure() : m_connected(false) {}

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
    };

} // namespace canaspad