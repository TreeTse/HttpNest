#pragma once
#include <vector>
#include <atomic>
#include "base/NoCopyable.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

namespace nest
{
    namespace net
    {
        using EventLoopThreadPtr = std::shared_ptr<EventLoopThread>;

        class EventLoopThreadPool: public base::NonCopyable
        {
        public:
            EventLoopThreadPool(int threadNum, int start = 0, int cpus = 4);
            ~EventLoopThreadPool();

            std::vector<EventLoop *> GetLoops() const;
            EventLoop *GetNextLoop();
            size_t Size();
            void Start();
        private:
            std::vector<EventLoopThreadPtr> threads_;
            std::atomic_int32_t loopIndex_{0};
        };
    }
}