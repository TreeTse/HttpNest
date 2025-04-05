#pragma once

#include <string>
#include <vector>

namespace nest
{
    namespace base
    {
        class StringUtils
        {
        public:
            static bool StartWith(const std::string &s, const std::string &sub);
            static bool EndWith(const std::string &s, const std::string &sub);
            static std::string FilePath(const std::string &path);
            static std::string FileNameExt(const std::string &path);
            static std::string FileName(const std::string &path);
            static std::string Extension(const std::string &path);
            static std::vector<std::string> SplitString(const std::string &s, const std::string &delimiter);
            static std::vector<std::string> SplitStringWithFSM(const std::string &s, const char delimiter);
        };
    }
}