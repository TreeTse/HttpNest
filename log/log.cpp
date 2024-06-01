#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
using namespace std;

Log::Log()
{
    lineCount_ = 0;
    isAsync_ = false;
}

Log::~Log()
{
    if (fp_ != NULL) {
        fclose(fp_);
    }
}

// Async need to set the length of blocking queue
bool Log::Init(const char *file_name, int close_log, int log_buf_size, int split_lines, int max_queue_size)
{
    if (max_queue_size >= 1) {
        isAsync_ = true;
        logQueue_ = new BlockQueue<string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid, NULL, FlushLogThread, NULL);
    }
    
    optCloseLog_ = close_log;
    logBufSize_ = log_buf_size;
    buf_ = new char[logBufSize_];
    memset(buf_, '\0', logBufSize_);
    splitLines_ = split_lines;// maximum number of rows

    time_t t = time(NULL);
    struct tm *sysTm = localtime(&t);
    struct tm myTm = *sysTm;

    const char *p = strrchr(file_name, '/');
    char logFullName[256] = {0};
    if (p == NULL) {
        snprintf(logFullName, 255, "%d_%02d_%02d_%s", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, file_name);
    } else {
        strcpy(logName_, p + 1);
        strncpy(dirName_, file_name, p - file_name + 1);
        snprintf(logFullName, 255, "%s%d_%02d_%02d_%s", dirName_, myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday, logName_);
    }

    today_ = myTm.tm_mday;
    
    fp_ = fopen(logFullName, "a");
    if (fp_ == NULL) {
        return false;
    }

    return true;
}

/*
 * Format the system information and output：format time + format content
 */
void Log::WriteLog(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, NULL);
    time_t t = now.tv_sec;
    struct tm *sysTm = localtime(&t);
    struct tm myTm = *sysTm;
    char s[16] = {0};
    switch (level) {
        case 0:
            strcpy(s, "[debug]:");
            break;
        case 1:
            strcpy(s, "[info]:");
            break;
        case 2:
            strcpy(s, "[warn]:");
            break;
        case 3:
            strcpy(s, "[erro]:");
            break;
        default:
            strcpy(s, "[info]:");
            break;
    }
    mutex_.Lock();
    lineCount_++;

    if (today_ != myTm.tm_mday || lineCount_ % splitLines_ == 0) {//everyday log
        char newLog[256] = {0};
        fflush(fp_);
        fclose(fp_);
        char tail[16] = {0};
        // Format the time in the log name
        snprintf(tail, 16, "%d_%02d_%02d_", myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday);
        // Update today_ and lineCount_
        if (today_ != myTm.tm_mday) {
            snprintf(newLog, 255, "%s%s%s", dirName_, tail, logName_);
            today_ = myTm.tm_mday;
            lineCount_ = 0;
        } else {// If the maximum line is exceeded, add a suffix to the previous log name, lineCount_/splitLines_
            snprintf(newLog, 255, "%s%s%s.%lld", dirName_, tail, logName_, lineCount_ / splitLines_);
        }
        fp_ = fopen(newLog, "a");
    }
 
    mutex_.Unlock();

    va_list valst;
    va_start(valst, format);

    string logStr;
    mutex_.Lock();

    // Time formatting
    int n = snprintf(buf_, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                     myTm.tm_year + 1900, myTm.tm_mon + 1, myTm.tm_mday,
                     myTm.tm_hour, myTm.tm_min, myTm.tm_sec, now.tv_usec, s);
    //content formatting
    int m = vsnprintf(buf_ + n, logBufSize_ - 1, format, valst);
    buf_[n + m] = '\n';
    buf_[n + m + 1] = '\0';
    logStr = buf_;

    mutex_.Unlock();

    if (isAsync_ && !logQueue_->Full()) {
        logQueue_->Push(logStr);
    } else {
        mutex_.Lock();
        fputs(logStr.c_str(), fp_);
        mutex_.Unlock();
    }

    va_end(valst);
}

void Log::Flush(void)
{
    mutex_.Lock();
    fflush(fp_);
    mutex_.Unlock();
}
