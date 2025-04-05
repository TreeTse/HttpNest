#pragma once

#include "Connection.h"
#include "network/base/InetAddress.h"
#include "network/base/MsgBuffer.h"
#include <functional>
#include <memory>
#include <list>
#include <sys/uio.h>

namespace nest
{
    namespace net
    {
        class TcpConnection;
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using CloseConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
        using MessageCallback = std::function<void(const TcpConnectionPtr &, MsgBuffer &buffer)>;
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
        using TimeoutCallback = std::function<void(const TcpConnectionPtr &)>;
        struct TimeoutEntry;
        class TcpConnection: public Connection
        {
        public:
            TcpConnection(EventLoop *loop, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);
            virtual ~TcpConnection();

            void OnClose() override;
            void SetCloseCallback(const CloseConnectionCallback &cb);
            void SetCloseCallback(CloseConnectionCallback &&cb);
            void SetRecvMsgCallback(const MessageCallback &cb);
            void SetRecvMsgCallback(MessageCallback &&cb);
            void SetWriteCompleteCallback(const WriteCompleteCallback &cb);
            void SetWriteCompleteCallback(WriteCompleteCallback &&cb);
            void SetTimeoutCallback(int timeout, const TimeoutCallback &cb);
            void SetTimeoutCallback(int timeout, TimeoutCallback &&cb);
            void ForceClose() override;
            void OnRead() override;
            void OnError(const std::string &msg) override;
            void OnWrite() override;
            void Send(std::list<BufferNodePtr> &list);
            void Send(const char *buf, size_t size);
            void OnTimeout();
            void EnableCheckIdleTimeout(int32_t maxTime);

        private:
            void SendInLoop(const char *buf, size_t size);
            void SendInLoop(std::list<BufferNodePtr> &list);
            void ExtendLife();

            CloseConnectionCallback closeCb_;
            MessageCallback messageCb_;
            WriteCompleteCallback writeCompleteCb_;
            bool isClosed_{false};
            MsgBuffer msgBuffer_;
            std::vector<struct iovec> iovecList_;
            int32_t maxIdleTime_{30};
            std::weak_ptr<TimeoutEntry> timeoutEntry_;
        };
        struct TimeoutEntry
        {
            TimeoutEntry(const TcpConnectionPtr &c)
            : conn(c)
            {}
            ~TimeoutEntry()
            {
                auto c = conn.lock();
                if (c) {
                    c->OnTimeout();
                }
            }
            std::weak_ptr<TcpConnection> conn;    
        };
    }
}