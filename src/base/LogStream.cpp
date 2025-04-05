#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>
#include "LogStream.h"
#include "Time.h"

using namespace nest::base;

const char *log_string[] = {
    " TRACE ",
    " DEBUG ",
    " INFO ",
    " WARN ",
    " ERROR "
};

static thread_local pid_t thread_id = 0;

Logger *nest::base::g_logger = nullptr;

LogStream::LogStream(Logger *loger, const char *file, int line, LogLevel level, const char *func)
: logger_(loger) {
    const char *filename = strrchr(file, '/');
    if (filename) {
        filename = filename + 1;
    } else {
        filename = file;
    }
    
    stream_ << Time::ISOTime() << " ";
    if (thread_id == 0) {
        thread_id =  static_cast<pid_t>(::syscall(SYS_gettid));
    }
    stream_ << thread_id;
    stream_ << log_string[level];
    stream_ << "[" << filename << ":" << line << "]";
    if (func) {
        stream_ << "[" << func << "]";
    }
}

LogStream::~LogStream()
{
    stream_ << "\n";
    if (logger_) {
        logger_->Write(stream_.str());
    } else {
        std::cout << stream_.str() << std::endl;
    }
}