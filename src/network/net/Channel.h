#pragma once
#include <string>
#include <sys/epoll.h>
#include <memory>

namespace nest
{
    namespace net
    {
        class EventLoop;
        const int kEventRead = (EPOLLIN|EPOLLPRI|EPOLLET);
        const int kEventWrite = (EPOLLOUT|EPOLLET);
        class Channel: public std::enable_shared_from_this<Channel>
        {
            friend class EventLoop;
        public:
            Channel(EventLoop *loop);
            Channel(EventLoop *loop, int fd);
            virtual ~Channel();

            virtual void OnRead() {};
            virtual void OnWrite() {};
            virtual void OnClose() {};
            virtual void OnError(const std::string &msg) {};
            bool EnableWriting(bool enable);
            bool EnableReading(bool enable);
            int fd() const;
            void Close();
            void SetRevents(uint32_t ev);
            uint32_t Revents();
            void HandleEvent();
            uint32_t Event();
        protected:
            EventLoop *loop_{nullptr};
            int fd_{-1};
            uint32_t event_{0};
            uint32_t revents_ = 0;
            bool inEpoll_{false};
        };        
    }
}