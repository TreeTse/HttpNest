#pragma once
#include "http/HttpHandler.h"
#include "network/net/TcpConnection.h"
#include "network/TcpServer.h"

namespace nest
{
    namespace mm
    {
        using namespace nest::net;
        class HttpServer: public TcpServer
        {
        public:
            HttpServer(EventLoop *loop, const InetAddress &local, HttpHandler *handler = nullptr);
            ~HttpServer();

            void Start() override;
            void Stop() override;

        private:
            void OnNewConnection(const TcpConnectionPtr &conn);
            void OnDestroyed(const TcpConnectionPtr &conn);
            void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf);
            void OnWriteComplete(const ConnectionPtr &con);
            void OnActive(const ConnectionPtr &conn);

            HttpHandler *httpHandler_{nullptr};
        };
    }
}