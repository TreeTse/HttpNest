#include "EventLoopThreadPool.h"
#include <pthread.h>

using namespace nest::net;

namespace
{
    void BindCpu(std::thread &t, int n)
    {
        cpu_set_t cpu;

        CPU_ZERO(&cpu);
        CPU_SET(n, &cpu);

        pthread_setaffinity_np(t.native_handle(), sizeof(cpu), &cpu);
    }
}

EventLoopThreadPool::EventLoopThreadPool(int threadNum, int start, int cpus)
{
    if(threadNum <= 0) {
        threadNum = 1;
    }
    for(int i = 0; i < threadNum; i++) {
        threads_.emplace_back(std::make_shared<EventLoopThread>());
        if(cpus > 0) {
            int n = (start + i) % cpus;
            BindCpu(threads_.back()->Thread(), n);
        }
    }
}

EventLoopThreadPool::~EventLoopThreadPool()
{}

std::vector<EventLoop *> EventLoopThreadPool::GetLoops() const
{
    std::vector<EventLoop *> result;
    for(auto &t:threads_) {
        result.emplace_back(t->Loop());
    }
    return result;
}

EventLoop *EventLoopThreadPool::GetNextLoop()
{
    int index = loopIndex_;
    loopIndex_++;
    return threads_[index % threads_.size()]->Loop();
}

size_t EventLoopThreadPool::Size()
{
    return threads_.size();
}

void EventLoopThreadPool::Start()
{
    for(auto &t:threads_) {
        t->Run();
    }
}