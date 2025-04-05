#pragma once
#include <functional>
#include "network/net/UdpSocket.h"

namespace nest
{
    namespace net
    {
        using ConnectedCallback = std::function<void (const UdpSocketPtr &, bool)>;
        class UdpClient:public UdpSocket
        {
        public:
            UdpClient(EventLoop *loop, const InetAddress &server);
            virtual ~UdpClient();

            void SetConnectedCallback(const ConnectedCallback &cb);
            void SetConnectedCallback(ConnectedCallback &&cb);
            void Connect();
            void ConnectInLoop();
            void OnClose() override;
            void Send(std::list<BufferNodePtr> &list);
            void Send(const char *buf, size_t size);
        
        private:
            InetAddress servAddr_;
            bool connected_{false};
            struct sockaddr_in6 sockAddr_;
            socklen_t sockLen_{sizeof(struct sockaddr_in6)};
            ConnectedCallback connectedCb_;
        };
    }
}