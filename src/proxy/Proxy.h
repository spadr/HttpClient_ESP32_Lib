#pragma once
#include <string>
#include <optional>
#include "../core/Request.h"
#include "../core/Connection.h"
#include "../core/CommonTypes.h"

namespace canaspad
{

    class Proxy
    {
    public:
        Proxy(const ClientOptions &options);

        bool isEnabled() const;
        void applyProxySettings(Request &request);
        bool establishTunnel(Connection *connection, const Request &request);

    private:
        struct ProxySettings
        {
            std::string host;
            int port;
            std::string username;
            std::string password;
        };

        std::optional<ProxySettings> m_proxySettings;

        void parseProxyUrl(const std::string &proxyUrl);
    };

} // namespace canaspad