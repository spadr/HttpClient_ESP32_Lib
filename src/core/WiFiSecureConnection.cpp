#include "WiFiSecureConnection.h"

namespace canaspad
{

    WiFiSecureConnection::WiFiSecureConnection() = default;
    WiFiSecureConnection::~WiFiSecureConnection() = default;

    bool WiFiSecureConnection::isConnectionValid(const std::string &host,
                                                 int port) const
    {
        auto now = std::chrono::steady_clock::now();
        return (now - m_lastUsed <= m_keepAliveTimeout) && (m_connectedHost == host) &&
               (m_connectedPort == port);
    }

    bool WiFiSecureConnection::checkTimeout(
        const std::chrono::steady_clock::time_point &start,
        const std::chrono::milliseconds &timeout) const
    {
        return (std::chrono::steady_clock::now() - start) > timeout;
    }

    bool WiFiSecureConnection::connect(const std::string &host, int port)
    {
        // Keep-Alive接続のチェック
        if (isConnected() && isConnectionValid(host, port))
        {
            return true;
        }

        // 新しい接続を確立
        auto connectStart = std::chrono::steady_clock::now();

        if (m_verifySsl)
        {
            if (!m_caCert.empty())
            {
                setCACert(m_caCert.c_str());
            }

            if (!m_clientCert.empty())
            {
                setCertificate(m_clientCert.c_str());
            }

            if (!m_privateKey.empty())
            {
                setPrivateKey(m_privateKey.c_str());
            }
        }
        else
        {
            setInsecure();
        }

        setTimeout(m_connectTimeout.count());
        bool result = false;

        while (!result)
        {
            if (checkTimeout(connectStart, m_connectTimeout))
            {
                return false;
            }

            result =
                WiFiClientSecure::connect(host.c_str(), port); // WiFiClientSecure::connect を呼び出す
            if (!result)
            {
                _lastError = getLastError();
                // 短い遅延を入れて再試行
                delay(100);
            }
        }

        if (result)
        {
            // 接続成功時に最終使用時刻と接続情報を更新
            m_lastUsed = std::chrono::steady_clock::now();
            m_connectedHost = host;
            m_connectedPort = port;
        }

        return result;
    }

    void WiFiSecureConnection::disconnect() { stop(); }

    bool WiFiSecureConnection::isConnected() const
    {
        return connected(); // WiFiSecureConnection::connected() を呼び出す
    }

    size_t WiFiSecureConnection::write(const uint8_t *buf, size_t size)
    {
        setTimeout(m_writeTimeout.count() /
                   1000); // WiFiSecureConnection::setTimeout() を呼び出す
        return WiFiClientSecure::write(buf, size);
    }

    int WiFiSecureConnection::read(uint8_t *buf, size_t size)
    {
        setTimeout(m_readTimeout.count() /
                   1000); // WiFiSecureConnection::setTimeout() を呼び出す
        return WiFiClientSecure::read(buf, size);
    }

    // read(size_t size) 関数を追加
    std::string WiFiSecureConnection::read(size_t size)
    {
        setTimeout(m_readTimeout.count() /
                   1000); // WiFiSecureConnection::setTimeout() を呼び出す
        std::string result;
        result.reserve(size);
        while (result.length() < size && available())
        {
            char c = WiFiClientSecure::read();
            result += c;
        }
        return result;
    }

    void WiFiSecureConnection::setTimeouts(const std::chrono::milliseconds &connectTimeout,
                                           const std::chrono::milliseconds &readTimeout,
                                           const std::chrono::milliseconds &writeTimeout)
    {
        // ミリ秒から秒に変換（切り上げ）
        m_connectTimeout =
            std::chrono::duration_cast<std::chrono::seconds>(connectTimeout +
                                                             std::chrono::milliseconds(999)) /
            1000;
        m_readTimeout =
            std::chrono::duration_cast<std::chrono::seconds>(readTimeout +
                                                             std::chrono::milliseconds(999)) /
            1000;
        m_writeTimeout =
            std::chrono::duration_cast<std::chrono::seconds>(writeTimeout +
                                                             std::chrono::milliseconds(999)) /
            1000;

        // WiFiClientSecure のタイムアウトを設定
        WiFiClientSecure::setTimeout(
            std::max({m_connectTimeout, m_readTimeout, m_writeTimeout}).count());
    }

    std::string WiFiSecureConnection::readLine()
    {
        setTimeout(m_readTimeout.count() /
                   1000); // WiFiSecureConnection::setTimeout() を呼び出す
        return readStringUntil('\n').c_str();
    }

    void WiFiSecureConnection::setVerifySsl(bool verify) { m_verifySsl = verify; }

    void WiFiSecureConnection::setCACert(const char *rootCA)
    {
        if (rootCA != nullptr)
        {
            m_caCert = std::string(rootCA);
            WiFiClientSecure::setCACert(m_caCert.c_str());
        }
        else
        {
            m_caCert.clear();
            WiFiClientSecure::setCACert(nullptr);
        }
    }

    void WiFiSecureConnection::setClientCert(const char *cert)
    {
        if (cert != nullptr)
        {
            m_clientCert = std::string(cert);
        }
        else
        {
            m_clientCert.clear();
        }
    }

    void WiFiSecureConnection::setClientPrivateKey(const char *private_key)
    {
        if (private_key != nullptr)
        {
            m_privateKey = std::string(private_key);
        }
        else
        {
            m_privateKey.clear();
        }
    }

    bool WiFiSecureConnection::connected() const
    {
        return const_cast<WiFiClientSecure *>(static_cast<const WiFiClientSecure *>(this))
            ->connected();
    }

    int WiFiSecureConnection::setTimeout(uint32_t seconds)
    {
        return WiFiClientSecure::setTimeout(seconds);
    }

    int WiFiSecureConnection::available() { return WiFiClientSecure::available(); }

    int WiFiSecureConnection::read() { return WiFiClientSecure::read(); }

} // namespace canaspad