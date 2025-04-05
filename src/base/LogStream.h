#pragma once
#include <sstream>
#include "Logger.h"

namespace nest
{
    namespace base
    {
        extern Logger *g_logger;
        class LogStream
        {
        public:
            LogStream(Logger *loger, const char *file, int line, LogLevel level, const char *func = nullptr);
            ~LogStream();
            template<class T> LogStream & operator<<(const T& value)
            {
                stream_ << value;
                return *this;
            }

        private:
            std::ostringstream stream_;
            Logger *logger_{nullptr};
        };
    }
}

#define LOG_TRACE \
    if (g_logger && nest::base::g_logger->GetLogLevel() <= nest::base::kTrace) \
        nest::base::LogStream(nest::base::g_logger, __FILE__, __LINE__, nest::base::kTrace, __func__)
#define LOG_DEBUG \
    if (g_logger && nest::base::g_logger->GetLogLevel() <= nest::base::kDebug) \
        nest::base::LogStream(nest::base::g_logger, __FILE__, __LINE__, nest::base::kDebug, __func__)
#define LOG_INFO \
    if (g_logger && nest::base::g_logger->GetLogLevel() <= nest::base::kInfo) \
        nest::base::LogStream(nest::base::g_logger, __FILE__, __LINE__, nest::base::kInfo, __func__)
#define LOG_WARN \
        nest::base::LogStream(nest::base::g_logger, __FILE__, __LINE__, nest::base::kWarn, __func__)
#define LOG_ERROR \
        nest::base::LogStream(nest::base::g_logger, __FILE__, __LINE__, nest::base::kError, __func__)
    