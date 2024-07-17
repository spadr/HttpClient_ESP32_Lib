// ConnectionPool.h
#pragma once
#include "NetworkLayer.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <functional>

namespace http
{

    class ConnectionPool
    {
    public:
        ConnectionPool(size_t maxConnections = 10, std::chrono::seconds maxIdleTime = std::chrono::seconds(60));
        ~ConnectionPool();

        std::shared_ptr<INetworkLayer> getConnection(const std::string &url,
                                                     std::function<std::shared_ptr<WiFiSecureClient>()> createConnection);
        void releaseConnection(const std::string &url, std::shared_ptr<INetworkLayer> connection);

    private:
        struct PooledConnection
        {
            std::shared_ptr<INetworkLayer> connection;
            std::chrono::steady_clock::time_point lastUsed;
        };

        std::unordered_map<std::string, PooledConnection> m_pool;
        size_t m_maxConnections;
        std::chrono::seconds m_maxIdleTime;
        std::mutex m_mutex;

        void cleanupIdleConnections();
    };

} // namespace http