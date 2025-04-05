#pragma once

#include <sys/epoll.h>
#include <vector>
#include <unistd.h>
#include <unordered_map>
#include "Channel.h"

namespace nest
{
    namespace net
    {
        using ChannelPtr = std::shared_ptr<Channel>;
        class Epoller
        {
        public:
            explicit Epoller(int maxEvent = 1024);
            ~Epoller();
            std::vector<ChannelPtr> Poll(int timeoutMs = -1);
            void AddChannel(const ChannelPtr &ch);
            void RemoveChannel(const ChannelPtr &ch);
            void UpdateChannel(const ChannelPtr &ch);

        private:
            int fd_{-1};
            std::vector<struct epoll_event> events_;
            std::unordered_map<int, ChannelPtr> channels_;
        };
    }
}