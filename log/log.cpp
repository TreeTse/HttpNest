#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include "log.h"
#include <pthread.h>
using namespace std;

Log::Log(): lineCount_(0), fp_(nullptr), today_(0), isOpen_(false) {}

Log::~Log()
{
    if (fp_) {
        std::lock_guard<std::mutex> locker(mtx_);
        Flush();
        fclose(fp_);
    }
}

bool Log::IsOpen()
{
    return isOpen_;
}

int Log::GetLevel()
{
    return level_;
}

void Log::AppendLogLevelTitle(int level)
{
    switch (level) {
        case 0:
            buff_.Append("[debug]: ", 9);
            break;
        case 1:
            buff_.Append("[info]: ", 8);
            break;
        case 2:
            buff_.Append("[warn]: ", 8);
            break;
        case 3:
            buff_.Append("[error]: ", 9);
            break;
        default:
            buff_.Append("[info]: ", 8);
            break;
    }
}

void Log::Init(int level, const char* path, const char* suffix, int splitLines)
{
    isOpen_ = true;
    level_ = level;
    splitLines_ = splitLines;// maximum number of rows

    time_t t = time(nullptr);
    struct tm *sysTm = localtime(&t);

    path_ = path;
    suffix_ = suffix;
    char logFullName[256] = {0};
    snprintf(logFullName, 255, "%s/%04d_%02d_%02d%s",
             path_, sysTm->tm_year + 1900, sysTm->tm_mon + 1, sysTm->tm_mday, suffix_);
    today_ = sysTm->tm_mday;
    
    {
        std::lock_guard<std::mutex> locker(mtx_);
        if (fp_) {
            Flush();
            fclose(fp_);
        }
        fp_ = fopen(logFullName, "a");
        if (!fp_) {
            mkdir(path_, 0777);
            fp_ = fopen(logFullName, "a");
        }
        assert(fp_ != nullptr);
    }
}

/*
 * Format the system information and output：format time + format content
 */
void Log::WriteLog(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTm = localtime(&tSec);

    if (today_ != sysTm->tm_mday || (lineCount_ && (lineCount_ % splitLines_) == 0)) {//everyday log
        char newLog[256] = {0};
        char tail[16] = {0};
        // Format the time in the log name
        snprintf(tail, 16, "%04d_%02d_%02d", sysTm->tm_year + 1900, sysTm->tm_mon + 1, sysTm->tm_mday);
        // Update today_ and lineCount_
        if (today_ != sysTm->tm_mday) {
            snprintf(newLog, 255, "%s/%s%s", path_, tail, suffix_);
            today_ = sysTm->tm_mday;
            lineCount_ = 0;
        } else {// If the maximum line is exceeded, add a suffix to the previous log name, lineCount_/splitLines_
            snprintf(newLog, 255, "%s/%s-%d%s", path_, tail, (lineCount_ / splitLines_), suffix_);
        }
        std::unique_lock<std::mutex> locker(mtx_);
        Flush();
        fclose(fp_);
        fp_ = fopen(newLog, "a");
        assert(fp_ != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(mtx_);
        lineCount_++;

        // Time formatting
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         sysTm->tm_year + 1900, sysTm->tm_mon + 1, sysTm->tm_mday,
                         sysTm->tm_hour, sysTm->tm_min, sysTm->tm_sec, now.tv_usec);
        buff_.HasWritten(n);
        AppendLogLevelTitle(level);

        va_list valst;
        va_start(valst, format);
        //content formatting
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, valst);
        va_end(valst);
        buff_.HasWritten(m);
        buff_.Append("\n\0", 2);
        fputs(buff_.Peek(), fp_);
        buff_.RetrieveAll();
    }
}

void Log::Flush()
{
    fflush(fp_);
}
