#pragma once
#include <functional>
#include <memory>
#include "network/base/InetAddress.h"
#include "network/net/TcpConnection.h"
#include "network/net/EventLoop.h"

namespace nest
{
    namespace net
    {
        enum 
        {
            kTcpConStatusInit = 0,
            kTcpConStatusConnecting = 1,
            kTcpConStatusConnected = 2,
            kTcpConStatusDisConnected = 3,
        };
        using ConnectionCallback = std::function<void(const TcpConnectionPtr &con, bool)>;
        class TcpClient: public TcpConnection
        {
        public:
            TcpClient(EventLoop *loop, const InetAddress &server);
            virtual ~TcpClient();

            void SetConnectCallback(const ConnectionCallback &cb);
            void SetConnectCallback(ConnectionCallback &&cb);

            void Connect();

            void OnRead() override;
            void OnWrite() override;
            void OnClose() override;

            void Send(std::list<BufferNodePtr> &list);
            void Send(const char *buf, size_t size);

        private:
            void ConnectInLoop();
            void UpdateConnectionStatus();
            bool CheckError();
            InetAddress servAddr_;
            ConnectionCallback connectedCb_;
            int32_t status_{kTcpConStatusInit};
        };
    }
}