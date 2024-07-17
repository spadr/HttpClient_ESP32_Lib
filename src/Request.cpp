// Request.cpp
#include "Request.h"

namespace http
{

    Request::Request() : m_method(Method::GET) {}

    Request &Request::setUrl(const std::string &url)
    {
        m_url = url;
        return *this;
    }

    Request &Request::setMethod(Method method)
    {
        m_method = method;
        return *this;
    }

    Request &Request::addHeader(const std::string &key, const std::string &value)
    {
        m_headers[key] = value;
        return *this;
    }

    Request &Request::setBody(const std::string &body)
    {
        m_body = body;
        return *this;
    }

    Request &Request::setMultipartFormData(const std::vector<std::pair<std::string, std::string>> &formData)
    {
        m_multipartFormData = formData;
        return *this;
    }

    const std::string &Request::getUrl() const
    {
        return m_url;
    }

    Request::Method Request::getMethod() const
    {
        return m_method;
    }

    const std::unordered_map<std::string, std::string> &Request::getHeaders() const
    {
        return m_headers;
    }

    const std::string &Request::getBody() const
    {
        return m_body;
    }

    const std::vector<std::pair<std::string, std::string>> &Request::getMultipartFormData() const
    {
        return m_multipartFormData;
    }

} // namespace http