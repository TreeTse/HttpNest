#pragma once
#include "Channel.h"
#include <memory>

namespace nest
{
    namespace net
    {
        class PipeEvent: public Channel
        {
        public:
            PipeEvent(EventLoop *loop);
            ~PipeEvent();

            void OnRead() override;
            void OnClose() override;
            void OnError(const std::string &msg) override;
            void Write(const char *data, size_t len);
        private:
            int writefd_{-1};
        };
        using PipeEventPtr = std::shared_ptr<PipeEvent>;
    }
}