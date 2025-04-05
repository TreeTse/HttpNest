#include <sstream>
#include "FileMgr.h"
#include "Time.h"
#include "StringUtils.h"

using namespace nest::base;


namespace
{
    static nest::base::FileLogPtr file_log_nullptr;
}
void FileMgr::OnCheck()
{
    bool dayChange{false};
    bool hourChange{false};
    bool minuteChange{false};
    int year = 0, month = 0, day = -1, hour = -1, minute = 0, second = 0;
    Time::Now(year, month, day, hour, minute, second);

    if (lastDay_ == -1) {
        lastDay_ = day;
        lastHour_ = hour;
        lastMinute_ = minute;
        lastYear_ = year;
        lastMonth_ = month;
    }
    if (lastDay_ != day) {
        dayChange = true;
    }
    if (lastHour_ != hour) {
        hourChange = true;
    }
    if (lastMinute_ != minute) {
        minuteChange = true;
    }
    if (!dayChange && !hourChange && !minuteChange) {
        return;
    }
    
    std::lock_guard<std::mutex> gd(mtx_);
    for (auto &log : logs_) {
        if (minuteChange && log.second->GetRotateType() == kRotateMinute) {
            RotateMinutes(log.second);
        }
        if (hourChange && log.second->GetRotateType() == kRotateHour) {
            RotateHours(log.second);
        }
        if (dayChange && log.second->GetRotateType() == kRotateDay) {
            RotateDays(log.second);
        }
    }
    lastDay_ = day;
    lastHour_ = hour;
    lastMinute_ = minute;
    lastYear_ = year;
    lastMonth_ = month;
}

FileLogPtr FileMgr::GetFileLog(const std::string &filename)
{
    std::lock_guard<std::mutex> gd(mtx_);
    auto iter = logs_.find(filename);
    if (iter != logs_.end()) {
        return iter->second;
    }
    FileLogPtr log = std::make_shared<FileLog>();
    if (!log->Open(filename)) {
        return file_log_nullptr;
    }
    logs_.emplace(filename, log);
    return log;
}

void FileMgr::RemoveFileLog(const FileLogPtr &log)
{
    std::lock_guard<std::mutex> gd(mtx_);
    logs_.erase(log->FilePath());
}

void FileMgr::RotateDays(const FileLogPtr &file)
{
    if (file->FileSize() > 0) {
        char buf[128] = {0};
        sprintf(buf, "_%4d-%02d-%02d", lastYear_, lastMonth_, lastDay_);
        std::string filePath = file->FilePath();
        std::string path = StringUtils::FilePath(filePath);
        std::string fileName = StringUtils::FileName(filePath);
        std::string fileExt = StringUtils::Extension(filePath);

        std::ostringstream ss;
        ss << path
           << fileName
           << buf
           << "."
           << fileExt;
        file->Rotate(ss.str());   
    }
}

void FileMgr::RotateHours(const FileLogPtr &file)
{
    if (file->FileSize() > 0) {
        char buf[128] = {0};
        sprintf(buf, "_%4d-%02d-%02dT%02d", lastYear_, lastMonth_, lastDay_, lastHour_);
        std::string filePath = file->FilePath();
        std::string path = StringUtils::FilePath(filePath);
        std::string fileName = StringUtils::FileName(filePath);
        std::string fileExt = StringUtils::Extension(filePath);

        std::ostringstream ss;
        ss << path
           << fileName
           << buf
           << "."
           << fileExt;
        file->Rotate(ss.str());   
    }
}

void FileMgr::RotateMinutes(const FileLogPtr &file)
{
    if (file->FileSize() > 0) {
        char buf[128] = {0};
        sprintf(buf, "_%4d-%02d-%02dT%02d%02d", lastYear_, lastMonth_, lastDay_, lastHour_, lastMinute_);
        std::string filePath = file->FilePath();
        std::string path = StringUtils::FilePath(filePath);
        std::string fileName = StringUtils::FileName(filePath);
        std::string fileExt = StringUtils::Extension(filePath);

        std::ostringstream ss;
        ss << path
           << fileName
           << buf
           << "."
           << fileExt;
        file->Rotate(ss.str());   
    }
}