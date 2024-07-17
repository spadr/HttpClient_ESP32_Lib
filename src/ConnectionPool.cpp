// ConnectionPool.cpp
#include "ConnectionPool.h"

namespace http
{

    ConnectionPool::ConnectionPool(size_t maxConnections, std::chrono::seconds maxIdleTime)
        : m_maxConnections(maxConnections), m_maxIdleTime(maxIdleTime)
    {
    }

    ConnectionPool::~ConnectionPool()
    {
        // すべての接続を切断
        for (auto &[url, pooledConnection] : m_pool)
        {
            pooledConnection.connection->disconnect();
        }
    }

    std::shared_ptr<INetworkLayer> ConnectionPool::getConnection(const std::string &url,
                                                                 std::function<std::shared_ptr<WiFiSecureClient>()> createConnection)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        cleanupIdleConnections();

        auto it = m_pool.find(url);
        if (it != m_pool.end())
        {
            it->second.lastUsed = std::chrono::steady_clock::now();
            return it->second.connection;
        }

        if (m_pool.size() >= m_maxConnections)
        {
            // 最も古い接続を削除
            auto oldestIt = std::min_element(m_pool.begin(), m_pool.end(),
                                             [](const auto &a, const auto &b)
                                             {
                                                 return a.second.lastUsed < b.second.lastUsed;
                                             });
            m_pool.erase(oldestIt);
        }

        // 外部から受け取った関数を使用して WiFiSecureClient を生成
        auto newConnection = createConnection();
        m_pool[url] = {newConnection, std::chrono::steady_clock::now()};
        return newConnection;
    }

    void ConnectionPool::releaseConnection(const std::string &url, std::shared_ptr<INetworkLayer> connection)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_pool.find(url);
        if (it != m_pool.end() && it->second.connection == connection)
        {
            it->second.lastUsed = std::chrono::steady_clock::now();
        }
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

} // namespace http