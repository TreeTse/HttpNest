#include "threadpool.h"


ThreadPool::ThreadPool(size_t threadNum, const std::string& threadType): stop_(false), threadType_(threadType)
{
    for (size_t i = 0; i < threadNum; i++) {
        threads_.emplace_back([this] {
            LOG_INFO("Create %s thread(%d)", threadType_.c_str(), syscall(SYS_gettid));
            while (!stop_) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->mtx_);
                    this->con_.wait(lock, [this] {
                        return (this->stop_ == true || this->taskQueue_.empty() == false);
                    });

                    if ((this->stop_ == true) && (this->taskQueue_.empty() == true)) return;

                    task = std::move(this->taskQueue_.front());
                    this->taskQueue_.pop();                    
                }
                LOG_INFO("%s(%d) execute task", threadType_.c_str(), syscall(SYS_gettid));
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    Stop();
}

void ThreadPool::AddTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        taskQueue_.push(task);
    }
    con_.notify_one();
}

size_t ThreadPool::GetSize()
{
    return threads_.size();
}

void ThreadPool::Stop()
{
    if (stop_) return;

    stop_ = true;

    con_.notify_all();

    for (std::thread &t: threads_) {
        t.join();
    }
}