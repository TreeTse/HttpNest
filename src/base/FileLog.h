#pragma once
#include <string>
#include <memory>

namespace nest
{
    namespace base
    {
        enum RotateType
        {
            kRotateNone,
            kRotateMinute,
            kRotateHour,
            kRotateDay,
        };
        class FileLog
        {
        public:
            FileLog() = default;
            ~FileLog() = default;

            bool Open(const std::string &filePath);
            size_t WriteLog(const std::string &msg);
            void Rotate(const std::string &file);
            void SetRotate(RotateType type);
            RotateType GetRotateType() const;
            int64_t FileSize() const;
            std::string FilePath() const;
        private:
            int fd_{-1};
            RotateType rotateType_{kRotateNone};
            std::string filePath_;
        };
        using FileLogPtr = std::shared_ptr<FileLog>;
    }
}