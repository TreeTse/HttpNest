#pragma once
#include <functional>
#include <memory>
#include <unordered_set>
#include "network/net/TcpConnection.h"
#include "network/net/EventLoop.h"
#include "network/net/Acceptor.h"
#include "network/base/InetAddress.h"

namespace nest
{
    namespace net
    {
        class TcpServer
        {
        public:
            TcpServer(EventLoop *loop, const InetAddress &addr);
            virtual ~TcpServer();

            void SetNewConnectionCallback(const std::function<void(const TcpConnectionPtr &)> &cb);
            void SetNewConnectionCallback(std::function<void(const TcpConnectionPtr &)> &&cb);
            void SetDestroyConnectionCallback(const std::function<void(const TcpConnectionPtr&)> &cb);
            void SetDestroyConnectionCallback(std::function<void(const TcpConnectionPtr&)> &&cb);
            void SetActiveCallback(const ActiveCallback &cb);
            void SetActiveCallback(ActiveCallback &&cb);
            void SetWriteCompleteCallback(const WriteCompleteCallback &cb);
            void SetWriteCompleteCallback(WriteCompleteCallback &&cb);
            void SetMessageCallback(const MessageCallback &cb);
            void SetMessageCallback(MessageCallback &&cb);
            void OnAccet(int fd, const InetAddress &addr);
            void OnConnectionClose(const TcpConnectionPtr &con);

            virtual void Start();
            virtual void Stop();

        private:
            EventLoop *loop_{nullptr};
            InetAddress addr_;
            std::shared_ptr<Acceptor> acceptor_;
            std::function<void(const TcpConnectionPtr &)> newConnectionCb_;
            std::function<void(const TcpConnectionPtr &)> destroyConnectionCb_;
            ActiveCallback activeCb_;
            WriteCompleteCallback writeCompleteCb_;
            MessageCallback messageCb_;
            std::unordered_set<TcpConnectionPtr> connections_;
        };
    }
}

