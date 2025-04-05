#include "Logger.h"
#include <iostream>


using namespace nest::base;

Logger::Logger(const FileLogPtr &log)
: fileLog_(log)
{}

void Logger::SetLogLevel(const LogLevel &level)
{
    level_ = level;
}

LogLevel Logger::GetLogLevel() const
{
    return level_;
}

void Logger::Write(const std::string &msg)
{
    if (fileLog_) {
        fileLog_->WriteLog(msg);
    } else {
        std::cout << msg;
    }
}