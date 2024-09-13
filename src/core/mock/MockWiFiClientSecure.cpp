#include "MockWiFiClientSecure.h"
#include <thread>
#include <algorithm>

namespace canaspad
{

    // ClientOptions を引数にとるコンストラクタ
    MockWiFiClientSecure::MockWiFiClientSecure(const ClientOptions &options)
        : m_connected(false),
          m_verifySsl(options.verifySsl),
          m_caCert(options.rootCA),
          m_clientCert(options.clientCert),
          m_clientPrivateKey(options.clientPrivateKey),
          m_options(options)
    {
    }

    // ClientOptions を使用して設定を行うメソッド
    void MockWiFiClientSecure::setOptions(const ClientOptions &options)
    {
        m_verifySsl = options.verifySsl;
        m_caCert = options.rootCA;
        m_clientCert = options.clientCert;
        m_clientPrivateKey = options.clientPrivateKey;
    }

    bool MockWiFiClientSecure::connect(const std::string &host, int port)
    {
        switch (m_connectBehavior)
        {
        case ConnectBehavior::AlwaysSuccess:
            m_connected = true;
            return true;

        case ConnectBehavior::AlwaysFail:
            return false;

        case ConnectBehavior::FailNTimesThenSuccess:
            if (m_failCount > 0)
            {
                m_failCount--;
                return false;
            }
            else
            {
                m_connected = true;
                return true;
            }

        default:
            m_connected = true;
            return true;
        }
    }

    void MockWiFiClientSecure::setConnectBehavior(ConnectBehavior behavior, int failCount)
    {
        m_connectBehavior = behavior;
        m_failCount = failCount;
    }

    void MockWiFiClientSecure::disconnect() { m_connected = false; }

    bool MockWiFiClientSecure::isConnected() const { return m_connected; }

    size_t MockWiFiClientSecure::write(const uint8_t *buf, size_t size)
    {
        m_log.addSent(buf, size);
        return size;
    }

    int MockWiFiClientSecure::read(uint8_t *buf, size_t size)
    {
        // タイムアウトシミュレーション
        if (m_readBehavior == ReadBehavior::Timeout &&
            std::chrono::steady_clock::now() - m_operationStart > m_readTimeout)
        {
            return 0; // タイムアウトをシミュレート
        }

        // 読み込みシナリオに基づいて動作
        switch (m_readBehavior)
        {
        case ReadBehavior::Normal:
            break; // 通常の動作

        case ReadBehavior::SlowResponse:
            std::this_thread::sleep_for(m_slowResponseDelay);
            break;

        case ReadBehavior::DropConnection:
            m_connected = false; // 接続を切断
            return 0;

        default:
            break;
        }

        // 通常の read 処理 (データの読み込み)
        size_t readSize = std::min(size, m_receiveBuffer.size());
        std::copy(m_receiveBuffer.begin(), m_receiveBuffer.begin() + readSize, buf);
        m_receiveBuffer.erase(m_receiveBuffer.begin(), m_receiveBuffer.begin() + readSize);
        m_log.addReceived(buf, readSize);

        if (m_receiveBuffer.empty() && !m_responseQueue.empty())
        {
            m_receiveBuffer = m_responseQueue.front();
            m_responseQueue.pop();
        }

        return readSize;
    }

    void MockWiFiClientSecure::setReadBehavior(ReadBehavior behavior, std::chrono::milliseconds delay)
    {
        m_readBehavior = behavior;
        m_slowResponseDelay = delay;
    }

    int MockWiFiClientSecure::setTimeout(uint32_t seconds)
    {
        m_readTimeout = std::chrono::seconds(seconds);
        return 0; // 成功を返す
    }

    void MockWiFiClientSecure::setTimeouts(
        const std::chrono::milliseconds &connectTimeout,
        const std::chrono::milliseconds &readTimeout,
        const std::chrono::milliseconds &writeTimeout)
    {

        m_readTimeout = readTimeout;

        // 常に m_operationStart を更新
        m_operationStart = std::chrono::steady_clock::now();
    }

    std::string MockWiFiClientSecure::readLine()
    {
        std::string line;
        while (available() > 0)
        {
            char c = read();
            line += c;
            if (c == '\n')
            {
                break;
            }
        }
        return line;
    }

    std::string MockWiFiClientSecure::read(size_t size)
    {
        std::string result;
        result.reserve(size);
        for (size_t i = 0; i < size && available(); ++i)
        {
            result += read();
        }
        return result;
    }

    void MockWiFiClientSecure::setVerifySsl(bool verify)
    {
        m_verifySsl = verify;
    }

    void MockWiFiClientSecure::setCACert(const char *rootCA)
    {
        m_caCert = rootCA ? rootCA : "";
    }

    void MockWiFiClientSecure::setClientCert(const char *cert)
    {
        m_clientCert = cert ? cert : "";
    }

    void MockWiFiClientSecure::setClientPrivateKey(const char *privateKey)
    {
        m_clientPrivateKey = privateKey ? privateKey : "";
    }

    bool MockWiFiClientSecure::getVerifySsl() const
    {
        return m_verifySsl;
    }

    const std::string &MockWiFiClientSecure::getCACert() const
    {
        return m_caCert;
    }

    const std::string &MockWiFiClientSecure::getClientCert() const
    {
        return m_clientCert;
    }

    const std::string &MockWiFiClientSecure::getClientPrivateKey() const
    {
        return m_clientPrivateKey;
    }

    bool MockWiFiClientSecure::connected() const { return m_connected; }

    int MockWiFiClientSecure::available() { return m_receiveBuffer.size(); }

    int MockWiFiClientSecure::read()
    {
        if (m_receiveBuffer.empty())
        {
            return -1;
        }
        uint8_t data = m_receiveBuffer.front();
        m_receiveBuffer.erase(m_receiveBuffer.begin());
        m_log.addReceived(&data, 1);
        return data;
    }

    void MockWiFiClientSecure::injectResponse(const std::vector<uint8_t> &response)
    {
        m_responseQueue.push(response);
        if (m_receiveBuffer.empty() && !m_responseQueue.empty())
        {
            m_receiveBuffer = m_responseQueue.front();
            m_responseQueue.pop();
        }
    }

    const CommunicationLog &MockWiFiClientSecure::getCommunicationLog() const
    {
        return m_log;
    }

} // namespace canaspad