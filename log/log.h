#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class Log
{
public:
    static Log *GetInstance()
    {
        static Log instance;
        return &instance;
    }

    static void *FlushLogThread(void *args)
    {
        Log::GetInstance()->AsyncWriteLog();
    }
    
    bool Init(const char *file_name, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    void WriteLog(int level, const char *format, ...);

    void Flush(void);

private:
    Log();
    virtual ~Log();
    void *AsyncWriteLog()
    {
        string singleLog;
        while (logQueue_->Pop(singleLog)) {
            mutex_.Lock();
            fputs(singleLog.c_str(), fp_);
            mutex_.Unlock();
        }
    }

private:
    char dirName_[128];
    char logName_[128];
    int splitLines_;
    int logBufSize_;
    long long lineCount_;  // number of log lines
    int today_;
    FILE *fp_;
    char *buf_;
    BlockQueue<string> *logQueue_;
    bool isAsync_;
    Locker mutex_;
    int optCloseLog_;
};

#define LOG_DEBUG(format, ...) if(0 == optCloseLog_) {Log::GetInstance()->WriteLog(0, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}
#define LOG_INFO(format, ...) if(0 == optCloseLog_) {Log::GetInstance()->WriteLog(1, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}
#define LOG_WARN(format, ...) if(0 == optCloseLog_) {Log::GetInstance()->WriteLog(2, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}
#define LOG_ERROR(format, ...) if(0 == optCloseLog_) {Log::GetInstance()->WriteLog(3, format, ##__VA_ARGS__); Log::GetInstance()->Flush();}

#endif
