#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../core/CommonTypes.h"
#include "../utils/Utils.h"
#include "../utils/HttpMethod.h"

namespace canaspad
{

    struct MultipartPart
    {
        std::string name;
        std::string content;
        std::string filename;
        std::string contentType;
    };

    class Request
    {
    public:
        canaspad::HttpMethod m_method;

        Request();

        Request &setUrl(const std::string &url);
        Request &setMethod(canaspad::HttpMethod method);
        Request &addHeader(const std::string &key, const std::string &value);
        Request &setBody(const std::string &body);
        Request &setMultipartFormData(const std::vector<std::pair<std::string, std::string>> &formData);
        Request &addMultipartFile(const std::string &name,
                                  const std::string &filename,
                                  const std::string &content,
                                  const std::string &contentType = "application/octet-stream");

        const std::string &getUrl() const;
        canaspad::HttpMethod getMethod() const;
        const std::unordered_map<std::string, std::string> &getHeaders() const;
        const std::string &getBody() const;
        const std::vector<std::pair<std::string, std::string>> &getMultipartFormData() const;
        const std::vector<MultipartPart> &getMultipartParts() const;

    private:
        std::string m_url;
        std::unordered_map<std::string, std::string> m_headers;
        std::string m_body;
        std::vector<MultipartPart> m_multipartParts;
        mutable std::vector<std::pair<std::string, std::string>> m_multipartLegacyView;
    };

} // namespace canaspad