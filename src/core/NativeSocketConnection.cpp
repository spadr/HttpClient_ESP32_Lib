#ifdef ARDUINO_ARCH_NATIVE

#include "NativeSocketConnection.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <fcntl.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

namespace canaspad
{

#ifdef _WIN32
    bool NativeSocketConnection::s_wsaInitialized = false;
#endif

    NativeSocketConnection::NativeSocketConnection()
    {
#ifdef _WIN32
        m_socket = INVALID_SOCKET;
        if (!s_wsaInitialized)
        {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
            {
                s_wsaInitialized = true;
            }
        }
#else
        m_socket = -1;
#endif
    }

    NativeSocketConnection::~NativeSocketConnection()
    {
        disconnect();
    }

    bool NativeSocketConnection::connect(const std::string &host, int port)
    {
        disconnect();

#ifdef _WIN32
        struct addrinfo hints = {}, *res = nullptr;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        std::string portStr = std::to_string(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0)
        {
            return false;
        }

        m_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (m_socket == INVALID_SOCKET)
        {
            freeaddrinfo(res);
            return false;
        }

        // Set non-blocking mode for timeout handling
        u_long mode = 1;
        ioctlsocket(m_socket, FIONBIO, &mode);

        if (::connect(m_socket, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                freeaddrinfo(res);
                return false;
            }

            // Wait for connection with timeout
            fd_set set;
            FD_ZERO(&set);
            FD_SET(m_socket, &set);
            struct timeval tv;
            tv.tv_sec = m_connectTimeout.count() / 1000;
            tv.tv_usec = (m_connectTimeout.count() % 1000) * 1000;

            if (select(0, NULL, &set, NULL, &tv) <= 0)
            {
                closesocket(m_socket);
                m_socket = INVALID_SOCKET;
                freeaddrinfo(res);
                return false;
            }
        }

        // Restore blocking mode
        mode = 0;
        ioctlsocket(m_socket, FIONBIO, &mode);
        freeaddrinfo(res);

#else
        struct hostent *server = gethostbyname(host.c_str());
        if (server == nullptr)
            return false;

        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
            return false;

        struct sockaddr_in serverAddr;
        std::memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        std::memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
        serverAddr.sin_port = htons(port);

        // Set non-blocking
        int flags = fcntl(m_socket, F_GETFL, 0);
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);

        if (::connect(m_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
        {
            if (errno != EINPROGRESS)
            {
                close(m_socket);
                m_socket = -1;
                return false;
            }

            fd_set set;
            FD_ZERO(&set);
            FD_SET(m_socket, &set);
            struct timeval tv;
            tv.tv_sec = m_connectTimeout.count() / 1000;
            tv.tv_usec = (m_connectTimeout.count() % 1000) * 1000;

            if (select(m_socket + 1, NULL, &set, NULL, &tv) <= 0)
            {
                close(m_socket);
                m_socket = -1;
                return false;
            }
        }

        // Restore blocking
        fcntl(m_socket, F_SETFL, flags);
#endif

        m_connected = true;
        return true;
    }

    void NativeSocketConnection::disconnect()
    {
        if (m_socket !=
#ifdef _WIN32
            INVALID_SOCKET
#else
            -1
#endif
        )
        {
#ifdef _WIN32
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
#else
            close(m_socket);
            m_socket = -1;
#endif
        }
        m_connected = false;
    }

    bool NativeSocketConnection::isConnected() const
    {
        return m_connected; // 簡易的なチェック
    }

    size_t NativeSocketConnection::write(const uint8_t *buf, size_t size)
    {
        if (!isConnected())
            return 0;
#ifdef _WIN32
        int sent = ::send(m_socket, reinterpret_cast<const char *>(buf), (int)size, 0);
#else
        int sent = ::send(m_socket, buf, size, 0);
#endif
        return (sent >= 0) ? sent : 0;
    }

    int NativeSocketConnection::read(uint8_t *buf, size_t size)
    {
        if (!isConnected())
            return -1;

        // Check if data available with timeout
        fd_set set;
        FD_ZERO(&set);
        FD_SET(m_socket, &set);
        struct timeval tv;
        tv.tv_sec = m_readTimeout.count() / 1000;
        tv.tv_usec = (m_readTimeout.count() % 1000) * 1000;

#ifdef _WIN32
        int ret = select(0, &set, NULL, NULL, &tv);
#else
        int ret = select(m_socket + 1, &set, NULL, NULL, &tv);
#endif

        if (ret <= 0)
            return 0; // Timeout or error

#ifdef _WIN32
        int received = ::recv(m_socket, reinterpret_cast<char *>(buf), (int)size, 0);
#else
        int received = ::recv(m_socket, buf, size, 0);
#endif

        if (received == 0)
        {
            disconnect(); // Connection closed
            return 0;
        }
        if (received < 0)
            return -1;
        return received;
    }

    void NativeSocketConnection::setTimeouts(const std::chrono::milliseconds &connectTimeout,
                                             const std::chrono::milliseconds &readTimeout,
                                             const std::chrono::milliseconds &writeTimeout)
    {
        m_connectTimeout = connectTimeout;
        m_readTimeout = readTimeout;
        m_writeTimeout = writeTimeout;
    }

    std::string NativeSocketConnection::readLine()
    {
        std::string line;
        uint8_t c;
        while (read(&c, 1) > 0)
        {
            line += (char)c;
            if (c == '\n')
                break;
        }
        return line;
    }

    std::string NativeSocketConnection::read(size_t size)
    {
        std::string data;
        if (size == 0)
            return data;

        std::vector<uint8_t> buf(size);
        int readBytes = read(buf.data(), size);
        if (readBytes > 0)
        {
            data.assign(reinterpret_cast<char *>(buf.data()), readBytes);
        }
        return data;
    }

    bool NativeSocketConnection::connected() const
    {
        return isConnected();
    }

    int NativeSocketConnection::available()
    {
        if (!isConnected())
            return 0;

        // Use select to check readability without blocking
        fd_set set;
        FD_ZERO(&set);
        FD_SET(m_socket, &set);
        struct timeval tv = {0, 0};

#ifdef _WIN32
        int ret = select(0, &set, NULL, NULL, &tv);
        if (ret > 0)
        {
            u_long bytes = 0;
            if (ioctlsocket(m_socket, FIONREAD, &bytes) == 0)
            {
                return (int)bytes;
            }
        }
#else
        int ret = select(m_socket + 1, &set, NULL, NULL, &tv);
        if (ret > 0)
        {
            int bytes = 0;
            if (ioctl(m_socket, FIONREAD, &bytes) == 0)
            {
                return bytes;
            }
        }
#endif
        return 0;
    }

    int NativeSocketConnection::read()
    {
        uint8_t c;
        if (read(&c, 1) > 0)
            return c;
        return -1;
    }

    int NativeSocketConnection::setTimeout(uint32_t seconds)
    {
        m_readTimeout = std::chrono::seconds(seconds);
        return 0;
    }

} // namespace canaspad

#endif // ARDUINO_ARCH_NATIVE
