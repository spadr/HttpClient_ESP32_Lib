#include "Proxy.h"
#include "../utils/Utils.h"
#include <sstream>

namespace canaspad
{

    Proxy::Proxy(const ClientOptions &options)
    {
        if (!options.proxyUrl.empty())
        {
            parseProxyUrl(options.proxyUrl);
        }
    }

    bool Proxy::isEnabled() const
    {
        return m_proxySettings.has_value();
    }

    void Proxy::applyProxySettings(Request &request)
    {
        if (m_proxySettings)
        {
            request.setUrl("http://" + m_proxySettings->host + ":" + std::to_string(m_proxySettings->port) + request.getUrl());
        }
    }

    bool Proxy::establishTunnel(Connection *connection, const Request &request)
    {
        std::string connectRequest = "CONNECT " + Utils::extractHost(request.getUrl()) + ":" + std::to_string(Utils::extractPort(request.getUrl())) + " HTTP/1.1\r\n";
        connectRequest += "Host: " + Utils::extractHost(request.getUrl()) + ":" + std::to_string(Utils::extractPort(request.getUrl())) + "\r\n";

        if (!m_proxySettings->username.empty() && !m_proxySettings->password.empty())
        {
            std::string auth = m_proxySettings->username + ":" + m_proxySettings->password;
            connectRequest += "Proxy-Authorization: Basic " + Utils::base64Encode(auth) + "\r\n";
        }

        connectRequest += "\r\n";

        if (connection->write(reinterpret_cast<const uint8_t *>(connectRequest.c_str()), connectRequest.length()) != connectRequest.length())
        {
            return false;
        }

        std::string response = connection->readLine();
        if (response.find("200") == std::string::npos)
        {
            return false;
        }

        while (connection->readLine() != "\r\n")
        {
        }

        return true;
    }

    void Proxy::parseProxyUrl(const std::string &proxyUrl)
    {
        std::string::size_type protocolEnd = proxyUrl.find("://");
        if (protocolEnd == std::string::npos)
        {
            return;
        }

        std::string hostPort = proxyUrl.substr(protocolEnd + 3);
        std::string::size_type colonPos = hostPort.find(':');
        if (colonPos == std::string::npos)
        {
            return;
        }

        std::string host = hostPort.substr(0, colonPos);
        int port = std::stoi(hostPort.substr(colonPos + 1));

        m_proxySettings = ProxySettings{host, port, "", ""};

        std::string::size_type atPos = host.find('@');
        if (atPos != std::string::npos)
        {
            std::string userPass = host.substr(0, atPos);
            host = host.substr(atPos + 1);
            std::string::size_type colonPos = userPass.find(':');
            if (colonPos != std::string::npos)
            {
                m_proxySettings->username = userPass.substr(0, colonPos);
                m_proxySettings->password = userPass.substr(colonPos + 1);
            }
        }

        m_proxySettings->host = host;
        m_proxySettings->port = port;
    }

} // namespace canaspad