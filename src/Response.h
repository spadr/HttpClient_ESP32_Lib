// Response.h
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace http
{

    class Response
    {
    public:
        Response();

        int getStatusCode() const;
        const std::unordered_map<std::string, std::string> &getHeaders() const;
        const std::string &getBody() const;

        void setStatusCode(int code);
        void addHeader(const std::string &key, const std::string &value);
        void setBody(const std::string &body);
        void appendToBody(const std::string &data);

    private:
        int m_statusCode;
        std::unordered_map<std::string, std::string> m_headers;
        std::string m_body;
    };

} // namespace http