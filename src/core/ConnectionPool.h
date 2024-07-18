#pragma once
#include "Connection.h"
#include "../cookie/CookieJar.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <functional>
#include "../core/HttpResult.h"
#include "../core/CommonTypes.h"

namespace canaspad
{

    class ConnectionPool
    {
    public:
        ConnectionPool(const ClientOptions &options);
        ~ConnectionPool();

        std::shared_ptr<Connection> getConnection(const std::string &host, int port);
        void releaseConnection(const std::shared_ptr<Connection> &connection);
        std::shared_ptr<CookieJar> getCookieJar() const;
        void disconnectAll();

    private:
        struct PooledConnection
        {
            std::shared_ptr<Connection> connection;
            std::chrono::steady_clock::time_point lastUsed;
            std::string host;
            int port;
        };

        std::unordered_map<std::string, PooledConnection> m_pool;
        size_t m_maxConnections;
        std::chrono::seconds m_maxIdleTime;
        std::shared_ptr<CookieJar> m_cookieJar;
        std::mutex m_mutex;
        ClientOptions m_options;

        void cleanupIdleConnections();
        std::string generateConnectionKey(const std::string &host, int port);
    };

} // namespace canaspad