#include "TaskMgr.h"
#include "Time.h"

using namespace nest::base;

bool TaskMgr::Add(TaskPtr &task)
{
    std::lock_guard<std::mutex> gd(lock_);
    auto iter = tasks_.find(task);
    if (iter != tasks_.end()) {
        return false;
    }
    tasks_.emplace(task);
    return true;
}

bool TaskMgr::Del(TaskPtr &task)
{
    std::lock_guard<std::mutex> gd(lock_);
    tasks_.erase(task);
    return true;
}

void TaskMgr::OnWork()
{
    std::lock_guard<std::mutex> gd(lock_);
    int64_t now = Time::NowMS();
    for (auto iter = tasks_.begin(); iter != tasks_.end();) {
        if ((*iter)->When() < now) {
            (*iter)->Run();
            if ((*iter)->When() < now) {
                iter = tasks_.erase(iter);
                continue;
            }
        }
        iter++;
    }
}