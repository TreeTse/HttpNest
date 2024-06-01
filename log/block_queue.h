/************************************************************************************
 * Blocking queue implemented by circular array，m_back = (back_ + 1) % maxSize_;  
 ***********************************************************************************/

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std;

template <class T>
class BlockQueue
{
public:
    BlockQueue(int MaxSize = 1000)
    {
        if (MaxSize <= 0) {
            exit(-1);
        }
        maxSize_ = MaxSize;
        array_ = new T[MaxSize];
        size_ = 0;
        front_ = -1;
        back_ = -1;
    }

    void Clear()
    {
        mutex_.Lock();
        size_ = 0;
        front_ = -1;
        back_ = -1;
        mutex_.Unlock();
    }

    ~BlockQueue()
    {
        mutex_.Lock();
        if (array_ != NULL)
            delete [] array_;
        mutex_.Unlock();
    }

    bool Full() 
    {
        mutex_.Lock();
        if (size_ >= maxSize_){
            mutex_.Unlock();
            return true;
        }
        mutex_.Unlock();
        return false;
    }

    bool Empty() 
    {
        mutex_.Lock();
        if (0 == size_) {
            mutex_.Unlock();
            return true;
        }
        mutex_.Unlock();
        return false;
    }

    bool Front(T &value) 
    {
        mutex_.Lock();
        if (0 == size_) {
            mutex_.Unlock();
            return false;
        }
        value = array_[front_];
        mutex_.Unlock();
        return true;
    }

    bool Back(T &value) 
    {
        mutex_.Lock();
        if (0 == size_) {
            mutex_.Unlock();
            return false;
        }
        value = array_[back_];
        mutex_.Unlock();
        return true;
    }

    int Size() 
    {
        int tmp = 0;
        mutex_.Lock();
        tmp = size_;
        mutex_.Unlock();
        return tmp;
    }

    int MaxSize()
    {
        int tmp = 0;
        mutex_.Lock();
        tmp = maxSize_;
        mutex_.Unlock();
        return tmp;
    }

    bool Push(const T &item)
    {
        mutex_.Lock();
        if (size_ >= maxSize_) {
            cond_.Broadcast();
            mutex_.Unlock();
            return false;
        }
        back_ = (back_ + 1) % maxSize_;
        array_[back_] = item;
        size_++;
        cond_.Broadcast();
        mutex_.Unlock();
        return true;
    }

    bool Pop(T &item)
    {
        mutex_.Lock();
        while (size_ <= 0) {
            if (!cond_.Wait(mutex_.Get())) {
                mutex_.Unlock();
                return false;
            }
        }
        front_ = (front_ + 1) % maxSize_;
        item = array_[front_];
        size_--;
        mutex_.Unlock();
        return true;
    }

    bool Pop(T &item, int ms_timeout)
    {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);
        mutex_.Lock();
        if (size_ <= 0) {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            if (!cond_.Timewait(mutex_.Get(), t)) {
                mutex_.Unlock();
                return false;
            }
        }
        if (size_ <= 0) {
            mutex_.Unlock();
            return false;
        }
        front_ = (front_ + 1) % maxSize_;
        item = array_[front_];
        size_--;
        mutex_.Unlock();
        return true;
    }

private:
    Locker mutex_;
    Cond cond_;

    T *array_;
    int size_;
    int maxSize_;
    int front_;
    int back_;
};

#endif
