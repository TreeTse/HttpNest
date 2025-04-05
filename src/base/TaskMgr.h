#pragma once

#include <unordered_set>
#include <mutex>
#include "NoCopyable.h"
#include "Task.h"

namespace nest
{
    namespace base
    {
        class TaskMgr: public NonCopyable 
        {
        public:
            TaskMgr() = default;
            ~TaskMgr() = default;

            bool Add(TaskPtr &task);
            bool Del(TaskPtr &task);
            void OnWork();
        private:
            std::unordered_set<TaskPtr> tasks_;
            std::mutex lock_;
        };
    }
    #define sTaskMgr nest::base::Singleton<nest::base::TaskMgr>::Instance()
}