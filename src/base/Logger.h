#pragma once

#include <string>
#include "NoCopyable.h"
#include "FileLog.h"

namespace nest
{
    namespace base
    {
        enum LogLevel
        {
            kTrace,
            kDebug,
            kInfo,
            kWarn,
            kError,
            kMaxNumOfLogLevel,
        };

        class Logger: public NonCopyable
        {
        public:
            Logger(const FileLogPtr &log);
            ~Logger() = default;
            void SetLogLevel(const LogLevel &level);
            LogLevel GetLogLevel() const;
            void Write(const std::string &msg);
        private:
            LogLevel level_{kDebug};
            FileLogPtr fileLog_;
        };
    }
}