#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../mysql/sql_connection_pool.h"

template <typename T>
class ThreadPool
{
public:
    // Thread_number default 8，max_request default 10000
    ThreadPool(int actor_model, ConnectionPool *connPool, int thread_number = 8, int max_request = 100000);
    ~ThreadPool();
    bool Append(T *request, int state);
    bool Append(T *request);

private:
    // Continuously fetch tasks from the work queue and execute
    static void *Worker(void *arg);
    void Run();

private:
    int threadNum_;
    int maxRequests_;
    pthread_t *threads_;
    std::list<T *> workQueue_;
    Locker queueLocker_;
    Sem queueStat_;
    ConnectionPool *connPool_;
    int actorModel_;
};

template <typename T>
ThreadPool<T>::ThreadPool(int actor_model, ConnectionPool *connPool, int thread_number, int max_requests)
: actorModel_(actor_model), connPool_(connPool), threadNum_(thread_number), maxRequests_(max_requests), threads_(NULL)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    threads_ = new pthread_t[threadNum_];
    if (!threads_)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i) {
        /* Worker should be a static function */
        if (pthread_create(threads_ + i, NULL, Worker, this) != 0) {
            delete[] threads_;
            throw std::exception();
        }
        if (pthread_detach(threads_[i])) {
            delete[] threads_;
            throw std::exception();
        }
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
    delete[] threads_;
}

template <typename T>
bool ThreadPool<T>::Append(T *request, int state)
{
    queueLocker_.Lock();
    if (workQueue_.size() >= maxRequests_) {
        queueLocker_.Unlock();
        return false;
    }
    request->state_ = state;
    workQueue_.push_back(request);
    queueLocker_.Unlock();
    queueStat_.Post();
    return true;
}

template <typename T>
bool ThreadPool<T>::Append(T *request)
{
    queueLocker_.Lock();
    if (workQueue_.size() >= maxRequests_) {
        queueLocker_.Unlock();
        return false;
    }
    workQueue_.push_back(request);
    queueLocker_.Unlock();
    queueStat_.Post();
    return true;
}

template <typename T>
void *ThreadPool<T>::Worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    pool->Run();
    return pool;
}

template <typename T>
void ThreadPool<T>::Run()
{
    while (true) {
        queueStat_.Wait();
        queueLocker_.Lock();
        if (workQueue_.empty()) {
            queueLocker_.Unlock();
            continue;
        }
        T *request = workQueue_.front();
        workQueue_.pop_front();
        queueLocker_.Unlock();
        if (!request)
            continue;
        if (1 == actorModel_) {
            if (0 == request->state_) {
                if (request->ReadOnce()) {
                    request->improv_ = 1;
                    ConnectionRAII mysqlConn(&request->mysql_, connPool_);
                    request->Process();
                } else {
                    request->improv_ = 1;
                    request->timerFlag_ = 1;
                }
            } else {
                if (request->Write()) {
                    request->improv_ = 1;
                } else {
                    request->improv_ = 1;
                    request->timerFlag_ = 1;
                }
            }
        } else {
            ConnectionRAII mysqlConn(&request->mysql_, connPool_);
            request->Process();
        }
    }
}
#endif
