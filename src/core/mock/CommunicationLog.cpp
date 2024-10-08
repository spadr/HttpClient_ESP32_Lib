#include "CommunicationLog.h"

namespace canaspad
{

    void CommunicationLog::addSent(const std::uint8_t *buf, size_t size)
    {
        log.push_back({Entry::Type::Sent, SourceType::Client, std::vector<std::uint8_t>(buf, buf + size)});
    }

    void CommunicationLog::addReceived(const std::uint8_t *buf, size_t size)
    {
        log.push_back({Entry::Type::Received, SourceType::Server, std::vector<std::uint8_t>(buf, buf + size)});
    }

    const std::vector<CommunicationLog::Entry> &CommunicationLog::getLog() const
    {
        return log;
    }

} // namespace canaspad