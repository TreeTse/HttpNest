#pragma once

#include <functional>
#include <memory>
#include "network/TcpClient.h"
#include "network/net/EventLoop.h"
#include "network/base/InetAddress.h"
#include "http/HttpHandler.h"
#include "http/HttpRequest.h"

namespace nest
{
    namespace mm
    {
        using namespace nest::net;
        using TcpClientPtr = std::shared_ptr<TcpClient>;

        class HttpClient
        {
        public:
            HttpClient(EventLoop *loop, HttpHandler *handler);
            ~HttpClient();

            void SetCloseCallback(const CloseConnectionCallback &cb);
            void SetCloseCallback(CloseConnectionCallback &&cb);

            void Get(const std::string &url);
            void Post(const std::string &url, const PacketPtr &packet);

        private:  
            void OnWriteComplete(const TcpConnectionPtr &conn);
            void OnConnection(const TcpConnectionPtr& conn, bool connected);
            void OnMessage(const TcpConnectionPtr& conn, MsgBuffer &buf);        
            bool ParseUrl(const std::string &url);
            void CreateTcpClient();

            EventLoop *loop_{nullptr};
            InetAddress addr_;
            HttpHandler *handler_{nullptr};
            TcpClientPtr tcpClient_;
            std::string url_;
            bool isPost_{false};
            CloseConnectionCallback closeCb_;
            HttpRequestPtr request_;
            PacketPtr outPacket_;
        };
    }
}