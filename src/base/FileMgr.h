#pragma once

#include <unordered_map>
#include <mutex>
#include "FileLog.h"
#include "Singleton.h"

namespace nest
{
    namespace base
    {
        class FileMgr: public NonCopyable
        {
        public:
            FileMgr() = default;
            ~FileMgr() = default;

            void OnCheck();
            FileLogPtr GetFileLog(const std::string &filename);
            void RemoveFileLog(const FileLogPtr &log);
            void RotateDays(const FileLogPtr &file);
            void RotateHours(const FileLogPtr &file);
            void RotateMinutes(const FileLogPtr &file);
        private:
            std::unordered_map<std::string, FileLogPtr> logs_;
            std::mutex mtx_;
            int lastDay_{-1};
            int lastHour_{-1};
            int lastMinute_{-1};
            int lastYear_{-1};
            int lastMonth_{-1};
        };
    }
}

#define sFileMgr nest::base::Singleton<nest::base::FileMgr>::Instance()