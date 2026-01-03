#pragma once

#ifdef IS_NATIVE
// cpp-httplib はヘッダオンリーライブラリ
// Windows環境でのマクロ競合回避のための定義などが必要な場合がある
// #define CPPHTTPLIB_OPENSSL_SUPPORT // HTTP通信のみ使用するため無効化
#include <httplib.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>

class MockServerHelper
{
    std::string host;
    int port;

public:
    MockServerHelper(std::string h = "localhost", int p = 1080) : host(h), port(p) {}

    // 基本的なモック作成
    void setupMock(const std::string &path, const std::string &method, const std::string &responseBody, int statusCode = 200)
    {
        std::string json = "{"
                           "\"httpRequest\": { \"method\": \"" +
                           method + "\", \"path\": \"" + path + "\" },"
                                                                "\"httpResponse\": { \"body\": \"" +
                           escapeJson(responseBody) + "\", \"statusCode\": " + std::to_string(statusCode) + " }"
                                                                                                            "}";
        sendExpectation(json);
    }

    // 遅延付きモック作成
    void setupMockWithDelay(const std::string &path, const std::string &method, const std::string &responseBody, int statusCode, int delayMs)
    {
        std::string json = "{"
                           "\"httpRequest\": { \"method\": \"" +
                           method + "\", \"path\": \"" + path + "\" },"
                                                                "\"httpResponse\": { \"body\": \"" +
                           escapeJson(responseBody) + "\", \"statusCode\": " + std::to_string(statusCode) + ","
                                                                                                            "\"delay\": { \"timeUnit\": \"MILLISECONDS\", \"value\": " +
                           std::to_string(delayMs) + " } }"
                                                     "}";
        sendExpectation(json);
    }

    // リダイレクト用モック作成
    void setupMockRedirect(const std::string &path, const std::string &location)
    {
        std::string json = "{"
                           "\"httpRequest\": { \"method\": \"GET\", \"path\": \"" +
                           path + "\" },"
                                  "\"httpResponse\": { \"statusCode\": 302,"
                                  "\"headers\": { \"Location\": [\"" +
                           location + "\"] } }"
                                      "}";
        sendExpectation(json);
    }

    void reset()
    {
        httplib::Client cli(host, port);
        cli.Put("/mockserver/reset", "", "text/plain");
    }

private:
    void sendExpectation(const std::string &json)
    {
        httplib::Client cli(host, port);
        auto res = cli.Put("/mockserver/expectation", json, "application/json");
        if (!res || res->status != 201)
        {
            std::cerr << "Failed to setup mock expectation. Status: " << (res ? res->status : 0) << std::endl;
        }
    }

    // 簡易的なJSONエスケープ
    std::string escapeJson(const std::string &s)
    {
        std::ostringstream o;
        for (auto c : s)
        {
            switch (c)
            {
            case '"':
                o << "\\\"";
                break;
            case '\\':
                o << "\\\\";
                break;
            case '\b':
                o << "\\b";
                break;
            case '\f':
                o << "\\f";
                break;
            case '\n':
                o << "\\n";
                break;
            case '\r':
                o << "\\r";
                break;
            case '\t':
                o << "\\t";
                break;
            default:
                if ('\x00' <= c && c <= '\x1f')
                {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                }
                else
                {
                    o << c;
                }
            }
        }
        return o.str();
    }
};
#endif
