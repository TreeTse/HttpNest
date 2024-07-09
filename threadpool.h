#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <functional>
#include <thread>
#include <queue>
#include <atomic>
#include <sys/syscall.h>
#include <mutex>
#include <condition_variable>
#include "log/log.h"

class ThreadPool
{
public:
    ThreadPool(size_t threadNum, const std::string& threadType);

    ~ThreadPool();

    void AddTask(std::function<void()> task);

    size_t GetSize();

    void Stop();

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> taskQueue_;
    std::atomic_bool stop_;
    std::mutex mtx_;
    std::condition_variable con_;
    const std::string threadType_;
};


#endif