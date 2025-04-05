#include "TimingWheel.h"
#include <iostream>
#include "base/LogStream.h"

using namespace nest::net;

TimingWheel::TimingWheel()
: wheels_(4)
{
    wheels_[kTimingTypeSecond].resize(60);
    wheels_[kTimingTypeMinute].resize(60);
    wheels_[kTimingTypeHour].resize(24);
    wheels_[kTimingTypeDay].resize(30);
}

TimingWheel::~TimingWheel()
{}

void TimingWheel::InsertSecondEntry(uint32_t delay, EntryPtr entryPtr)
{
    wheels_[kTimingTypeSecond][delay - 1].emplace(entryPtr);
}

void TimingWheel::InsertMinuteEntry(uint32_t delay, EntryPtr entryPtr)
{
    auto minute = delay/kTimingMinute;
    auto second = delay%kTimingMinute;
    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr](){
        InsertEntry(second, entryPtr);
    });
    wheels_[kTimingTypeMinute][minute - 1].emplace(newEntryPtr);
}

void TimingWheel::InsertHourEntry(uint32_t delay, EntryPtr entryPtr)
{
    auto hour = delay/kTimingHour;
    auto second = delay%kTimingHour;
    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr](){
        InsertEntry(second, entryPtr);
    });
    wheels_[kTimingTypeHour][hour - 1].emplace(newEntryPtr);
}

void TimingWheel::InsertDayEntry(uint32_t delay, EntryPtr entryPtr)
{
    auto day = delay/kTimingDay;
    auto second = delay%kTimingDay;
    CallbackEntryPtr newEntryPtr = std::make_shared<CallbackEntry>([this, second, entryPtr](){
        InsertEntry(second, entryPtr);
    });
    wheels_[kTimingTypeDay][day - 1].emplace(newEntryPtr);
}

void TimingWheel::InsertEntry(uint32_t delay, EntryPtr entryPtr)
{
    if(delay <= 0) {
        entryPtr.reset();
    }
    if(delay < kTimingMinute) {
        InsertSecondEntry(delay, entryPtr);
    }
    else if(delay < kTimingHour) {
        InsertMinuteEntry(delay, entryPtr);
    }
    else if(delay < kTimingDay) {
        InsertHourEntry(delay, entryPtr);
    } else {
        auto day = delay/kTimingDay;
        if(day > 30) {
            LOG_ERROR << "It is not surpport > 30 day!";
            return;
        }
        InsertDayEntry(delay, entryPtr);
    }
}

void TimingWheel::OnTimer(int64_t now)
{
    if(lastTs_== 0) {
        lastTs_ = now;
    }
    if(now - lastTs_ < 1000) {
        return;
    }
    lastTs_ = now;
    ++tick_;
    PopUp(wheels_[kTimingTypeSecond]);
    if(tick_ % kTimingMinute == 0) {
        PopUp(wheels_[kTimingTypeMinute]);
    } else if(tick_ % kTimingHour == 0) {
        PopUp(wheels_[kTimingTypeHour]);
    } else if(tick_ % kTimingDay == 0) {
        PopUp(wheels_[kTimingTypeDay]);
    }
}

void TimingWheel::PopUp(Wheel &bq)
{
    WheelEntry tmp;
    bq.front().swap(tmp); // Memory is not reclaimed using clear()
    bq.pop_front();
    bq.push_back(WheelEntry());
}

void TimingWheel::RunAfter(double delay, const std::function<void()> &cb)
{
    CallbackEntryPtr cbEntry = std::make_shared<CallbackEntry>([cb](){
        cb();
    });
    InsertEntry(delay, cbEntry);
}

void TimingWheel::RunAfter(double delay, std::function<void()> &&cb)
{
    CallbackEntryPtr cbEntry = std::make_shared<CallbackEntry>([cb](){
        cb();
    });
    InsertEntry(delay, cbEntry);
}

void TimingWheel::RunEvery(double interval, const std::function<void()> &cb)
{
    CallbackEntryPtr cbEntry = std::make_shared<CallbackEntry>([this,cb,interval](){
        cb();
        RunEvery(interval, cb);
    });
    InsertEntry(interval, cbEntry);
}

void TimingWheel::RunEvery(double interval, std::function<void()> &&cb)
{
    CallbackEntryPtr cbEntry = std::make_shared<CallbackEntry>([this,cb,interval](){
        cb();
        RunEvery(interval, cb);
    });
    InsertEntry(interval, cbEntry);
}