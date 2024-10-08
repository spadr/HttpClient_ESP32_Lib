#pragma once

#include <cstdint>
#include <vector>

namespace canaspad
{

    class CommunicationLog
    {
    public:
        enum class SourceType
        {
            Client,
            Server
        };

        struct Entry
        {
            enum class Type
            {
                Sent,
                Received
            };
            Type type;
            SourceType source; // 送信元を追加
            std::vector<std::uint8_t> data;
        };

        void addSent(const std::uint8_t *buf, size_t size);
        void addReceived(const std::uint8_t *buf, size_t size);
        const std::vector<Entry> &getLog() const;

    private:
        std::vector<Entry> log;
    };

} // namespace canaspad