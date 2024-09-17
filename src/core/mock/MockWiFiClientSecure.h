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

    enum class ConnectBehavior
    {
        AlwaysSuccess,
        AlwaysFail,
        FailNTimesThenSuccess,
    };

    enum class ReadBehavior
    {
        Normal,
        SlowResponse,
        DropConnection,
        Timeout
    };

    enum class WriteBehavior
    {
        Normal,
        SlowResponse,
        DropConnection,
        Timeout
    };

    class MockWiFiClientSecure : public Connection
    {
    private:
        ClientOptions m_options;
        bool m_connected;
        CommunicationLog m_log;
        std::queue<std::vector<uint8_t>> m_responseQueue;
        std::vector<uint8_t> m_receiveBuffer;
        std::chrono::steady_clock::time_point m_operationStart;
        std::chrono::milliseconds m_readTimeout{0};

        // SSL 関連の設定を保持する変数
        bool m_verifySsl;
        std::string m_caCert;
        std::string m_clientCert;
        std::string m_clientPrivateKey;

        // 接続シナリオ
        ConnectBehavior m_connectBehavior{ConnectBehavior::AlwaysSuccess}; // デフォルトは必ず成功
        int m_failCount{0};                                                // FailNTimesThenSuccess の場合の失敗回数

        // 読み込みシナリオ
        ReadBehavior m_readBehavior{ReadBehavior::Normal}; // デフォルトは正常
        std::chrono::milliseconds m_slowResponseDelay{0};  // SlowResponse の場合の遅延時間

        // 書き込みシナリオ
        WriteBehavior m_writeBehavior{WriteBehavior::Normal}; // デフォルトは正常
        std::chrono::milliseconds m_writeDelay{0};            // 書き込み遅延時間

    public:
        MockWiFiClientSecure(const ClientOptions &options);

        bool connect(const std::string &host, int port) override;
        void disconnect() override;
        bool isConnected() const override;
        size_t write(const uint8_t *buf, size_t size) override;
        int read(uint8_t *buf, size_t size) override;
        int setTimeout(uint32_t seconds) override; // Deprecated
        void setTimeouts(const std::chrono::milliseconds &connectTimeout,
                         const std::chrono::milliseconds &readTimeout,
                         const std::chrono::milliseconds &writeTimeout) override; // Deprecated
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
        void setOptions(const ClientOptions &options);
        void setConnectBehavior(ConnectBehavior behavior, int failCount = 0);
        void setReadBehavior(ReadBehavior behavior, std::chrono::milliseconds delay = std::chrono::milliseconds(0));
        void setWriteBehavior(WriteBehavior behavior, std::chrono::milliseconds delay = std::chrono::milliseconds(0));

        // SSL 設定を確認するためのGetter メソッド
        bool getVerifySsl() const;
        const std::string &getCACert() const;
        const std::string &getClientCert() const;
        const std::string &getClientPrivateKey() const;
    };

} // namespace canaspad