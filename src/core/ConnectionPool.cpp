#include "ConnectionPool.h"

#include <algorithm>

#include "Connection.h"
#include "CommonTypes.h"
#include "WiFiSecureConnection.h"

namespace canaspad
{

    ConnectionPool::ConnectionPool(const ClientOptions &options,
                                   std::shared_ptr<Connection> connection)
        : m_maxConnections(10),
          m_maxIdleTime(std::chrono::seconds(60)),
          m_cookieJar(std::make_shared<CookieJar>()),
          m_options(options),
          m_defaultConnection(connection ? connection : std::make_shared<WiFiSecureConnection>())
    {
    }

    ConnectionPool::~ConnectionPool() { disconnectAll(); }

    std::shared_ptr<Connection> ConnectionPool::getConnection(
        const std::string &host, int port)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_defaultConnection)
        {
            return m_defaultConnection;
        }

        cleanupIdleConnections();

        std::string key = generateConnectionKey(host, port);
        auto it = m_pool.find(key);
        if (it != m_pool.end())
        {
            it->second.lastUsed = std::chrono::steady_clock::now();
            return it->second.connection;
        }

        if (m_pool.size() >= m_maxConnections)
        {
            // 最も古い接続を削除
            auto oldestIt = std::min_element(
                m_pool.begin(), m_pool.end(),
                [](const auto &a, const auto &b)
                { return a.second.lastUsed < b.second.lastUsed; });
            m_pool.erase(oldestIt);
        }

        // 新しい接続を作成
        auto newConnection = createNewConnection(host, port);
        m_pool[key] = {newConnection, std::chrono::steady_clock::now(), host, port};
        return newConnection;
    }

    std::shared_ptr<Connection> ConnectionPool::getConnection() const
    {
        if (m_defaultConnection)
        {
            return m_defaultConnection;
        }

        if (!m_pool.empty())
        {
            return m_pool.begin()->second.connection;
        }

        return nullptr;
    }

    std::shared_ptr<Connection> ConnectionPool::createNewConnection(const std::string &host, int port)
    {
        auto newConnection = std::make_shared<WiFiSecureConnection>();
        newConnection->setVerifySsl(m_options.verifySsl);
        if (m_options.verifySsl)
        {
            newConnection->setCACert(m_options.rootCA.c_str());
            newConnection->setClientCert(m_options.clientCert.c_str());
            newConnection->setClientPrivateKey(m_options.clientPrivateKey.c_str());
        }
        return newConnection;
    }

    void ConnectionPool::releaseConnection(
        const std::shared_ptr<Connection> &connection)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto &[key, pooledConnection] : m_pool)
        {
            if (pooledConnection.connection == connection)
            {
                pooledConnection.lastUsed = std::chrono::steady_clock::now();
                return;
            }
        }
    }

    std::shared_ptr<CookieJar> ConnectionPool::getCookieJar() const
    {
        return m_cookieJar;
    }

    void ConnectionPool::disconnectAll()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto &[key, pooledConnection] : m_pool)
        {
            pooledConnection.connection->disconnect();
        }
        m_pool.clear();
    }

    void ConnectionPool::cleanupIdleConnections()
    {
        auto now = std::chrono::steady_clock::now();
        for (auto it = m_pool.begin(); it != m_pool.end();)
        {
            if (now - it->second.lastUsed > m_maxIdleTime)
            {
                it->second.connection->disconnect();
                it = m_pool.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    std::string ConnectionPool::generateConnectionKey(const std::string &host,
                                                      int port)
    {
        return host + ":" + std::to_string(port);
    }

} // namespace canaspad