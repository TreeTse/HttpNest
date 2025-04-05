#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "network/net/Connection.h"
#include "network/TcpServer.h"
#include "network/net/EventLoopThreadPool.h"
#include "base/TaskMgr.h"
#include "base/Task.h"
#include "base/NoCopyable.h"
#include "base/Singleton.h"
#include "http/HttpHandler.h"

namespace nest
{
    namespace mm
    {
        using namespace nest::base;

        class HttpService: public HttpHandler
        {
        public:
            HttpService() = default;
            ~HttpService() = default;

            void OnNewConnection(const TcpConnectionPtr &conn) override;
            void OnConnectionDestroy(const TcpConnectionPtr &conn) override;
            void OnActive(const ConnectionPtr &conn) override;
            void OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data) override;
            void OnRecv(const TcpConnectionPtr &conn, const PacketPtr &data) override{};
            void OnSent(const TcpConnectionPtr &conn) override;
            bool OnSentNextChunk(const TcpConnectionPtr &conn) override;   
            void OnRequest(const TcpConnectionPtr &conn, const HttpRequestPtr &req, const PacketPtr &packet) override;

            void Start();
            void Stop();
        
        private:
            EventLoopThreadPool *pool_{nullptr};
            std::vector<TcpServer*> servers_;
            std::mutex lock_;
        };
        #define sMyService nest::base::Singleton<nest::mm::HttpService>::Instance()
    }
}