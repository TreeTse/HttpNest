#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <time.h>
#include <iostream>
#include <string>

class TimeStamp
{
public:
    TimeStamp();

    TimeStamp(int64_t timesec);

    ~TimeStamp();

    static TimeStamp Now();

    time_t Toint() const;

    std::string Tostring() const;

private:
    time_t curTimeSec_;
};



#endif