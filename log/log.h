#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <string>
#include <stdarg.h>
#include <thread>
#include <mutex>
#include <sys/stat.h>
#include "../buffer.h"

using namespace std;

class Log
{
public:
    static Log *GetInstance()
    {
        static Log instance;
        return &instance;
    }

    void Init(int level = 0, const char* path = "./log", const char* suffix = ".log",
              int splitLines = 5000000);

    int GetLevel();

    void WriteLog(int level, const char *format, ...);

    void Flush();

    bool IsOpen();

private:
    Log();

    virtual ~Log();

    void AppendLogLevelTitle(int level);

private:
    bool isOpen_;
    int level_;
    std::mutex mtx_;
    const char* path_;
    const char* suffix_;
    Buffer buff_;
    int splitLines_;
    long long lineCount_;  // number of log lines
    int today_;
    FILE *fp_;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::GetInstance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->WriteLog(level, format, ##__VA_ARGS__); \
            log->Flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif
