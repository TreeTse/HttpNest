#include "timestamp.h"


TimeStamp::TimeStamp()
{
    curTimeSec_ = time(0);
}

TimeStamp::TimeStamp(int64_t timesec): curTimeSec_(timesec)
{}

TimeStamp::~TimeStamp()
{}

TimeStamp TimeStamp::Now()
{
    return TimeStamp();
}

time_t TimeStamp::Toint() const
{
    return curTimeSec_;
}

std::string TimeStamp::Tostring() const
{
    char buf[32] = {0};
    tm *tm_time = localtime(&curTimeSec_);
    snprintf(buf, 20, "%4d-%02d-%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}