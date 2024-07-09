#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <functional>
#include <unistd.h>
#include <memory>
#include <queue>
#include <map>
#include <atomic>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <mutex>
#include "epoller.h"

class Channel;
class Epoller;
class Connection;
using spConnection = std::shared_ptr<Connection>;

class EventLoop
{
public:
    EventLoop(bool isMainloop, int timevalSec = 10, int timeoutSec = 20);

    ~EventLoop();

    void Run();

    void Stop();

    void UpdateChannel(Channel *ch);

    void RemoveChannel(Channel *ch);

    void SetEpollTimeoutCallback(std::function<void(EventLoop*)> cb);

    bool IsInLoopThread();

    void QueueInLoop(std::function<void()> cb);

    void WakeUp();

    void HandleWakeUp();

    void HandleTimer();

    void NewConnection(spConnection conn);

    void SetTimeoutCallback(std::function<void(int)> cb);

private:
    std::unique_ptr<Epoller> ep_;
    int wakeupfd_;
    std::function<void(EventLoop*)> epollTimeoutCallback_;
    std::unique_ptr<Channel> wakeChannel_;
    pid_t threadid_;
    std::mutex mtx_;
    std::queue<std::function<void()>> taskQueue_;
    int timevalSec_;
    int timeoutSec_;
    int timerfd_;
    std::unique_ptr<Channel> timerChannel_;
    bool isMainLoop_;
    std::map<int, spConnection> conns_;
    std::mutex connMtx_;
    std::function<void(int)> timeOutCallback_;
    std::atomic_bool stop_;
};


#endif