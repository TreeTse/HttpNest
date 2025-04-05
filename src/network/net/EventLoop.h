#pragma once
#include <memory>
#include <vector>
#include <sys/epoll.h>
#include <unordered_map>
#include <queue>
#include <functional>
#include <mutex>
#include "Channel.h"
#include "PipeEvent.h"
#include "TimingWheel.h"
#include "Epoller.h"

namespace nest
{
    namespace net
    {
        using ChannelPtr = std::shared_ptr<Channel>;
        class EventLoop
        {
        public:
            EventLoop();
            ~EventLoop();

            void Loop();
            void Quit();
            void AddChannel(const ChannelPtr &channel);
            void DelChannel(const ChannelPtr &channel);
            bool EnableChannelWriting(const ChannelPtr &channel, bool enable);
            bool EnableChannelReading(const ChannelPtr &channel, bool enable);
            void AssertInLoopThread();
            bool IsInLoopThread() const;
            void RunInLoop(const std::function<void()> &fn);
            void RunInLoop(std::function<void()> &&fn);

            void InsertEntry(uint32_t delay, EntryPtr entryPtr);
            void RunAfter(double delay, const std::function<void()> &cb);
            void RunAfter(double delay, std::function<void()> &&cb);
            void RunEvery(double interval, const std::function<void()> &cb);
            void RunEvery(double interval, std::function<void()> &&cb);

        private:
            void RunFuctions();
            void WakeUp();
            bool isLooping_{false};
            int epollfd_{-1};
            std::vector<struct epoll_event> epoll_events_;
            std::queue<std::function<void()>> fns_;
            std::mutex mtx_;
            PipeEventPtr pipeEvent_;
            TimingWheel wheel_;
            std::unique_ptr<Epoller> ep_;
        };
    }
}