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
        Serial.printf("MockWiFiClientSecure::write called. Size: %zu\n", size);

        m_writePerformed += 1; // write メソッドが呼ばれたことを記録

        // タイムアウトシミュレーション
        if (m_writeBehavior == WriteBehavior::Timeout &&
            std::chrono::steady_clock::now() - m_operationStart > m_writeDelay)
        {
            return 0; // タイムアウトをシミュレート
        }

        // 書き込みシナリオに基づいて動作
        switch (m_writeBehavior)
        {
        case WriteBehavior::Normal:
            break; // 通常の動作

        case WriteBehavior::SlowResponse:
            std::this_thread::sleep_for(m_writeDelay);
            break;

        case WriteBehavior::DropConnection:
            m_connected = false; // 接続を切断
            return 0;

        default:
            break;
        }

        m_log.addSent(buf, size);

        return size;
    }

    void MockWiFiClientSecure::setWriteBehavior(WriteBehavior behavior, std::chrono::milliseconds delay)
    {
        m_writeBehavior = behavior;
        m_writeDelay = delay;
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

        // m_receiveBuffer から \n を探す
        auto newlinePos = std::find(m_receiveBuffer.begin(), m_receiveBuffer.end(), '\n');

        if (newlinePos != m_receiveBuffer.end())
        {
            // \n が見つかった場合、行全体を line にコピー
            line = std::string(m_receiveBuffer.begin(), newlinePos + 1);

            // 1行読み込んだらログに追加
            m_log.addReceived(reinterpret_cast<const uint8_t *>(line.c_str()), line.size());

            // m_receiveBuffer から読み込んだ行を削除
            m_receiveBuffer.erase(m_receiveBuffer.begin(), newlinePos + 1);
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

    bool MockWiFiClientSecure::connected() const
    {
        return m_connected && !m_responses.empty();
    }

    int MockWiFiClientSecure::available()
    {
        // Serial.printf("MockWiFiClientSecure::available() called. Connected: %d, Responses: %zu\n", m_connected, m_responses.size());
        if (!connected() || m_responses.empty())
        {
            // Serial.println("MockWiFiClientSecure::available() - Not connected or no responses");
            return 0;
        }

        int availableBytes = m_responses.front().size() - m_currentResponsePos;
        // Serial.printf("MockWiFiClientSecure::available() - Available bytes: %d\n", availableBytes);
        return availableBytes;
    }

    void MockWiFiClientSecure::moveToNextResponse()
    {
        if (!m_responses.empty())
        {
            Serial.println("MockWiFiClientSecure::moveToNextResponse - Moving to next response");
            m_responses.pop_front();
            m_currentResponsePos = 0;
            m_readPerformed = m_writePerformed;
        }
        else
        {
            Serial.println("MockWiFiClientSecure::moveToNextResponse - No more responses available");
        }
    }

    int MockWiFiClientSecure::read()
    {
        Serial.printf("MockWiFiClientSecure::read() called. Connected: %d, Responses: %zu\n", m_connected, m_responses.size());
        if (!connected())
        {
            Serial.println("MockWiFiClientSecure::read() - Not connected or no responses");
            return -1;
        }

        const auto &currentResponse = m_responses.front();
        if (m_currentResponsePos >= currentResponse.size())
        {
            Serial.println("MockWiFiClientSecure::read() - End of current response");
            return -1;
        }

        uint8_t data = currentResponse[m_currentResponsePos++];
        m_log.addReceived(&data, 1);
        Serial.printf("MockWiFiClientSecure::read() - Read byte: %02X\n", data);
        return data;
    }

    int MockWiFiClientSecure::read(uint8_t *buf, size_t size)
    {
        m_readPerformed += 1; // read メソッドが呼ばれたことを記録
        Serial.printf("MockWiFiClientSecure::read called. Size: %zu\n", size);

        if (!connected())
        {
            return 0;
        }

        const auto currentResponse = m_responses.front();
        size_t bytesAvailable = currentResponse.size() - m_currentResponsePos;
        size_t bytesToRead = std::min(size, bytesAvailable);

        std::copy(currentResponse.begin() + m_currentResponsePos,
                  currentResponse.begin() + m_currentResponsePos + bytesToRead,
                  buf);

        m_currentResponsePos += bytesToRead;
        m_log.addReceived(buf, bytesToRead);

        return bytesToRead;
    }

    void MockWiFiClientSecure::injectResponse(const std::vector<uint8_t> &response)
    {
        m_responses.push_back(response);
    }

    const CommunicationLog &MockWiFiClientSecure::getCommunicationLog() const
    {
        return m_log;
    }

} // namespace canaspad