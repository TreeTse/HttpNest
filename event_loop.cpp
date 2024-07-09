#include "event_loop.h"

int CreateTimerfd(int sec = 30)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC|TFD_NONBLOCK);
    struct itimerspec spec;
    memset(&spec, 0, sizeof(struct itimerspec));
    spec.it_value.tv_sec = sec;
    spec.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0, &spec, 0);
    return tfd;
}

EventLoop::EventLoop(bool isMainloop, int timevalSec, int timeoutSec): ep_(new Epoller), wakeupfd_(eventfd(0, EFD_NONBLOCK)),
                wakeChannel_(new Channel(this, wakeupfd_)), isMainLoop_(isMainloop), timevalSec_(timevalSec), timeoutSec_(timeoutSec),
                timerfd_(CreateTimerfd(timevalSec_)), timerChannel_(new Channel(this, timerfd_)), stop_(false)
{
    wakeChannel_->SetReadCallback(std::bind(&EventLoop::HandleWakeUp, this));
    wakeChannel_->EnableReading();

    timerChannel_->SetReadCallback(std::bind(&EventLoop::HandleTimer, this));
    timerChannel_->EnableReading();
}

EventLoop::~EventLoop()
{}

void EventLoop::Run()
{
    LOG_DEBUG("EventLoop::Run() thread is %d", syscall(SYS_gettid));
    threadid_ = syscall(SYS_gettid);

    while (stop_ == false) {
        std::vector<Channel *> channels = ep_->Poll(10 * 1000);

        if (channels.size() == 0) {
            epollTimeoutCallback_(this);
        } else {
            for (auto &ch : channels) {
                ch->HandleEvent();
            }
        }
    }

    LOG_DEBUG("EventLoop::Run() end, thread is %d", syscall(SYS_gettid));
}

void EventLoop::Stop()
{
    stop_ = true;
    WakeUp();
}

void EventLoop::UpdateChannel(Channel *ch)
{
    ep_->UpdateChannel(ch);
}

void EventLoop::RemoveChannel(Channel *ch)
{
    ep_->RemoveChannel(ch);
}

void EventLoop::SetEpollTimeoutCallback(std::function<void(EventLoop*)> cb)
{
    epollTimeoutCallback_ = cb;
}

bool EventLoop::IsInLoopThread()
{
    return threadid_ == syscall(SYS_gettid);
}

void EventLoop::QueueInLoop(std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> gd(mtx_);
        taskQueue_.push(fn);
    }
    WakeUp();
}

void EventLoop::WakeUp()
{
    uint64_t val = 1;
    write(wakeupfd_, &val, sizeof(val));
}

void EventLoop::HandleWakeUp()
{
    uint64_t val;
    read(wakeupfd_, &val, sizeof(val));

    std::function<void()> fn;
    std::lock_guard<std::mutex> gd(mtx_);

    while (taskQueue_.size() > 0) {
        fn = std::move(taskQueue_.front());
        taskQueue_.pop();
        fn();
    }
}

void EventLoop::HandleTimer()
{
    struct itimerspec spec;
    memset(&spec, 0, sizeof(struct itimerspec));
    spec.it_value.tv_sec = timevalSec_;
    spec.it_value.tv_nsec = 0;
    timerfd_settime(timerfd_, 0, &spec, 0);

    if (isMainLoop_) {

    } else {
        time_t now = time(0);
        for (auto it = conns_.begin(); it != conns_.end();) {
            if (it->second->IsTimeOut(now, timeoutSec_)) {
                int connfd = it->first;
                {
                    std::lock_guard<std::mutex> gd(connMtx_);
                    it = conns_.erase(it);
                }
                timeOutCallback_(connfd);
            } else {
                it++;
            }
        }
    }
}

void EventLoop::NewConnection(spConnection conn)
{
    std::lock_guard<std::mutex> gd(connMtx_);
    conns_[conn->fd()] = conn;
}

void EventLoop::SetTimeoutCallback(std::function<void(int)> cb)
{
    timeOutCallback_ = cb;
}
