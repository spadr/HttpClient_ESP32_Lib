#include "MockWiFiClientSecure.h"

#include <algorithm>

#include "CommunicationLog.h"
#include "ResponseInjector.h"

namespace canaspad
{

    bool MockWiFiClientSecure::connect(const std::string &host, int port)
    {
        m_connected = true;
        return true;
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
        size_t readSize = std::min(size, m_receiveBuffer.size());
        std::copy(m_receiveBuffer.begin(), m_receiveBuffer.begin() + readSize, buf);
        m_receiveBuffer.erase(m_receiveBuffer.begin(),
                              m_receiveBuffer.begin() + readSize);
        m_log.addReceived(buf, readSize);
        return readSize;
    }

    int MockWiFiClientSecure::setTimeout(uint32_t seconds)
    {
        // モックではタイムアウトは無視されます
        return 0; // 成功を返す
    }

    void MockWiFiClientSecure::setTimeouts(
        const std::chrono::milliseconds &connectTimeout,
        const std::chrono::milliseconds &readTimeout,
        const std::chrono::milliseconds &writeTimeout)
    {
        // モックではタイムアウトは無視されます
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
        // モックではSSL検証は無視されます
    }

    void MockWiFiClientSecure::setCACert(const char *rootCA)
    {
        // モックではCA証明書は無視されます
    }

    void MockWiFiClientSecure::setClientCert(const char *cert)
    {
        // モックではクライアント証明書は無視されます
    }

    void MockWiFiClientSecure::setClientPrivateKey(const char *privateKey)
    {
        // モックでは秘密鍵は無視されます
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

    void MockWiFiClientSecure::injectResponse(
        const std::vector<uint8_t> &response)
    {
        m_injector.queueResponse(response);
    }

    const CommunicationLog &MockWiFiClientSecure::getCommunicationLog() const
    {
        return m_log;
    }

} // namespace canaspad