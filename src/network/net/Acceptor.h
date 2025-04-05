#pragma once

#include <functional>
#include "network/base/InetAddress.h"
#include "network/base/Socket.h"
#include "network/net/Channel.h"
#include "network/net/EventLoop.h"

namespace nest
{
    namespace net
    {
        using AcceptCallback = std::function<void(int sock, const InetAddress &addr)>;

        class Acceptor: public Channel
        {
        public:
            Acceptor(EventLoop *loop, const InetAddress &addr);
            ~Acceptor();

            void SetAcceptCallback(const AcceptCallback &cb);
            void SetAcceptCallback(AcceptCallback &&cb);
            void Start();
            void Stop();
            void OnRead() override;
            void OnError(const std::string &msg) override;
            void OnClose() override;
        private:
            void Open();
            InetAddress addr_;
            AcceptCallback acceptCb_;
            Socket *socketOpt_{nullptr};   
        };
    }
}