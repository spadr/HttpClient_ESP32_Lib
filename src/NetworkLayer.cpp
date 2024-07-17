// NetworkLayer.cpp
#include "NetworkLayer.h"

namespace http
{

    class WiFiSecureClient::Impl
    {
    public:
        MyWiFiSecureClient client;
        std::chrono::milliseconds connectTimeout{30000};
        std::chrono::milliseconds readTimeout{30000};
        std::chrono::milliseconds writeTimeout{30000};
        bool verifySsl = true;
        std::string caCert = "";
        std::string clientCert = "";
        std::string privateKey = "";

        // ID を追加
        int id;
        static int nextId;

        Impl() : id(nextId++) {}
    };

    int WiFiSecureClient::Impl::nextId = 0;

    WiFiSecureClient::WiFiSecureClient() : pImpl(std::make_unique<Impl>()) {}
    WiFiSecureClient::~WiFiSecureClient() = default;

    bool WiFiSecureClient::connect(const std::string &host, int port)
    {
        Serial.println("WiFiSecureClient::connect - Start");
        Serial.printf("[WiFiSecureClient %d] connect\n", pImpl->id);
        Serial.print("caCert (length): ");
        Serial.println(pImpl->caCert.length());

        if (0)
        {
            pImpl->client.setInsecure();
            Serial.println("Insecure connection");
        }
        else
        {
            if (0)
            {
                Serial.println("Setting CA Cert...");
                pImpl->client.setCACert(pImpl->caCert.c_str());
                Serial.print("caCert (length): ");
                Serial.println(pImpl->caCert.length());
                Serial.println("CA Cert set in connect");
            }

            if (0)
            {
                Serial.println("Setting Client Cert...");
                pImpl->client.setCertificate(pImpl->clientCert.c_str());
                Serial.print("clientCert (length): ");
                Serial.println(pImpl->clientCert.length());
                Serial.println("Client Cert set in connect");
            }

            if (0)
            {
                Serial.println("Setting Private Key...");
                pImpl->client.setPrivateKey(pImpl->privateKey.c_str());
                Serial.print("privateKey (length): ");
                Serial.println(pImpl->privateKey.length());
                Serial.println("Private Key set in connect");
            }
        }

        // pImpl->client.setTimeout(pImpl->connectTimeout.count());
        Serial.print("Host: ");
        Serial.println(host.c_str());
        Serial.print("Port: ");
        Serial.println(port);

        Serial.println("Attempting connection...");
        bool result = pImpl->client.connect(host.c_str(), port);
        Serial.print("Connection result: ");
        Serial.println(result ? "Success" : "Failure");

        if (!result)
        {
            Serial.print("Error code: ");
            Serial.println(pImpl->client.getLastError());
        }

        Serial.println("WiFiSecureClient::connect - End");
        return result;
    }

    void WiFiSecureClient::disconnect()
    {
        pImpl->client.stop();
    }

    bool WiFiSecureClient::isConnected() const
    {
        return pImpl->client.connected();
    }

    size_t WiFiSecureClient::write(const uint8_t *buf, size_t size)
    {
        pImpl->client.setTimeout(pImpl->writeTimeout.count());
        return pImpl->client.write(buf, size);
    }

    int WiFiSecureClient::read(uint8_t *buf, size_t size)
    {
        pImpl->client.setTimeout(pImpl->readTimeout.count());
        return pImpl->client.read(buf, size);
    }

    void WiFiSecureClient::setTimeouts(int connectTimeout, int readTimeout, int writeTimeout)
    {
        // ミリ秒から秒に変換（切り上げ）
        pImpl->connectTimeout = std::chrono::seconds((connectTimeout + 999) / 1000);
        pImpl->readTimeout = std::chrono::seconds((readTimeout + 999) / 1000);
        pImpl->writeTimeout = std::chrono::seconds((writeTimeout + 999) / 1000);

        // WiFiClientSecure のタイムアウトを設定
        pImpl->client.setTimeout(std::max({pImpl->connectTimeout, pImpl->readTimeout, pImpl->writeTimeout}).count());
    }

    std::string WiFiSecureClient::readLine()
    {
        pImpl->client.setTimeout(pImpl->readTimeout.count());
        return pImpl->client.readStringUntil('\n').c_str();
    }

    std::string WiFiSecureClient::read(size_t size)
    {
        pImpl->client.setTimeout(pImpl->readTimeout.count());
        std::string result;
        result.reserve(size);
        while (result.length() < size && pImpl->client.available())
        {
            char c = pImpl->client.read();
            result += c;
        }
        return result;
    }

    void WiFiSecureClient::setVerifySsl(bool verify)
    {
        pImpl->verifySsl = verify;
    }

    void WiFiSecureClient::setCACert(const char *rootCA)
    {
        Serial.println("WiFiSecureClient::setCACert");
        if (rootCA != nullptr)
        {
            pImpl->caCert = std::string(rootCA);
            pImpl->client.setCACert(pImpl->caCert.c_str());
            Serial.println("CA Cert set:");
            Serial.println(pImpl->caCert.length());
        }
        else
        {
            pImpl->caCert.clear();
            pImpl->client.setCACert(nullptr);
            Serial.println("CA Cert cleared");
        }
        Serial.println("-WiFiSecureClient::setCACert");
    }

    void WiFiSecureClient::setClientCert(const char *cert, const char *private_key)
    {
        Serial.println("WiFiSecureClient::setClientCert");
        Serial.printf("[WiFiSecureClient %d] setCACert\n", pImpl->id);
        if (cert != nullptr)
        {
            pImpl->clientCert = std::string(cert);
        }
        else
        {
            pImpl->clientCert.clear();
        }
        if (private_key != nullptr)
        {
            pImpl->privateKey = std::string(private_key);
        }
        else
        {
            pImpl->privateKey.clear();
        }
        Serial.println("-WiFiSecureClient::setClientCert");
    }

} // namespace http