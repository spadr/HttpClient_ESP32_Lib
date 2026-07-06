#include "Request.h"

namespace canaspad
{

    Request::Request() : m_method(canaspad::HttpMethod::GET) {}

    Request &Request::setUrl(const std::string &url)
    {
        m_url = url;
        return *this;
    }

    Request &Request::setMethod(canaspad::HttpMethod method)
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
        m_multipartParts.clear();
        for (const auto &field : formData)
        {
            MultipartPart part;
            part.name = field.first;
            part.content = field.second;
            m_multipartParts.push_back(std::move(part));
        }
        return *this;
    }

    Request &Request::addMultipartFile(const std::string &name,
                                       const std::string &filename,
                                       const std::string &content,
                                       const std::string &contentType)
    {
        MultipartPart part;
        part.name = name;
        part.filename = filename;
        part.content = content;
        part.contentType = contentType;
        m_multipartParts.push_back(std::move(part));
        return *this;
    }

    const std::string &Request::getUrl() const
    {
        return m_url;
    }

    canaspad::HttpMethod Request::getMethod() const
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
        m_multipartLegacyView.clear();
        for (const auto &part : m_multipartParts)
        {
            m_multipartLegacyView.emplace_back(part.name, part.content);
        }
        return m_multipartLegacyView;
    }

    const std::vector<MultipartPart> &Request::getMultipartParts() const
    {
        return m_multipartParts;
    }

} // namespace canaspad
