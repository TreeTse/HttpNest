#include "EventLoopThread.h"

using namespace nest::net;

EventLoopThread::EventLoopThread()
: thread_([this](){StartEventLoop();})
{}

EventLoopThread::~EventLoopThread()
{
    Run();
    if (loop_) {
        loop_->Quit();
    }
    
    if (thread_.joinable()) {
        thread_.join();
    }
}

void EventLoopThread::StartEventLoop()
{
    EventLoop loop;

    std::unique_lock<std::mutex> lk(mtx_);
    condition_.wait(lk, [this](){return isRunning_;});
    loop_ = &loop;
    promiseLoop_.set_value(1);
    loop.Loop();
    loop_ = nullptr;
}

void EventLoopThread::Run()
{
    std::call_once(once_, [this](){
        {
            std::lock_guard<std::mutex> lk(mtx_);
            isRunning_ = true;
            condition_.notify_all();
        }
        auto f = promiseLoop_.get_future();
        f.get(); //Ensure that loop_ are securely available externally
    });
}

EventLoop *EventLoopThread::Loop() const
{
    return loop_;
}

std::thread &EventLoopThread::Thread()
{
    return thread_;
}