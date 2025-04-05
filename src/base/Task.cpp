#include "Task.h"
#include "Time.h"

using namespace nest::base;

Task::Task(const TaskCallback &cb, int64_t interval)
: cb_(cb), interval_(interval), when_(interval_ + Time::NowMS())
{}

Task::Task(const TaskCallback &&cb, int64_t interval)
: cb_(std::move(cb)), interval_(interval), when_(interval_ + Time::NowMS())
{}

void Task::Run()
{
    if (cb_) {
        cb_(shared_from_this());
    }
}

void Task::Restart()
{
    when_ = interval_ + Time::NowMS();
}