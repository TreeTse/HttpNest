#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include "base/NoCopyable.h"
#include "EventLoop.h"

namespace nest
{
    namespace net
    {
        class EventLoopThread: public base::NonCopyable
        {
        public:
            EventLoopThread();
            ~EventLoopThread();

            void Run();
            EventLoop *Loop() const;
            std::thread &Thread();

        private:
            void StartEventLoop();

            EventLoop *loop_{nullptr};
            std::thread thread_;
            std::condition_variable condition_;
            std::mutex mtx_;
            bool isRunning_{false};
            std::once_flag once_;
            std::promise<int> promiseLoop_;
        };
    }
}