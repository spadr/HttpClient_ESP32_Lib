#include "Response.h"

namespace canaspad
{

    Response::Response() : m_statusCode(0) {}

    int Response::getStatusCode() const
    {
        return m_statusCode;
    }

    const std::unordered_map<std::string, std::string> &Response::getHeaders() const
    {
        return m_headers;
    }

    const std::string &Response::getBody() const
    {
        return m_body;
    }

    void Response::setStatusCode(int code)
    {
        m_statusCode = code;
    }

    void Response::addHeader(const std::string &key, const std::string &value)
    {
        m_headers[key] = value;
    }

    void Response::setBody(const std::string &body)
    {
        m_body = body;
    }

    void Response::appendToBody(const std::string &data)
    {
        m_body += data;
    }

} // namespace canaspad