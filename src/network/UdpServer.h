#pragma once
#include "network/net/UdpSocket.h"
#include "network/net/EventLoop.h"
#include "network/base/InetAddress.h"
#include "network/base/Socket.h"

namespace nest
{
    namespace net
    {
        class UdpServer: public UdpSocket
        {
        public:
            UdpServer(EventLoop *loop, const InetAddress &server);
            virtual ~UdpServer();

            void Start();
            void Stop();
        
        private:
            void Open();

            InetAddress server_;
        };
    }
}