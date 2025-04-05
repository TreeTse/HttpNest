#pragma once

#include <cstdint>
#include <string>

namespace nest
{
    namespace base
    {
        class Time
        {
        public:
            static int64_t NowMS();
            static int64_t Now();
            static int64_t Now(int &year, int &month, int &day, int &hour, int &minute, int &second);
            static std::string ISOTime(); 
        };
    }
}