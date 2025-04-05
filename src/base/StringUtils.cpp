#include "StringUtils.h"

using namespace nest::base;

bool StringUtils::StartWith(const std::string &s, const std::string &sub)
{
    if(sub.empty()) {
        return true;
    }
    if(s.empty()) {
        return false;
    }
    auto len = s.size();
    auto sublen = sub.size();
    if(len < sublen) return false;

    return s.compare(0, sublen, sub) == 0;
}

bool StringUtils::EndWith(const std::string &s, const std::string &sub)
{
    if(sub.empty()) {
        return true;
    }
    if(s.empty()) {
        return false;
    }
    auto len = s.size();
    auto sublen = sub.size();
    if(len < sublen) return false;

    return s.compare(len - sublen, sublen, sub) == 0;
}

std::string StringUtils::FilePath(const std::string &path)
{
    auto pos = path.find_last_of("/\\");
    if(pos != std::string::npos) {
        return path.substr(0, pos);
    } else {
        return "./";
    }
}

std::string StringUtils::FileNameExt(const std::string &path)
{
    auto pos = path.find_last_of("/\\");
    if(pos != std::string::npos) {
        if(pos + 1 < path.size()) {
            return path.substr(pos + 1);
        }
    }
    return path;
}

std::string StringUtils::FileName(const std::string &path)
{
    std::string fileName = FileNameExt(path);
    auto pos = fileName.find_last_of(".");
    if(pos != std::string::npos) {
        if(pos != 0) {
            return fileName.substr(0,pos);
        }
    }
    return fileName;
}

std::string StringUtils::Extension(const std::string &path)
{
    std::string fileName = FileNameExt(path);
    auto pos = fileName.find_last_of(".");
    if(pos != std::string::npos) {
        if(pos != 0 && pos + 1 < fileName.size()) {
            return fileName.substr(pos + 1);
        }
    }
    return std::string();
}

std::vector<std::string> StringUtils::SplitString(const std::string &s, const std::string &delimiter)
{
    if(delimiter.empty()) {
        return std::vector<std::string>{};
    }
    std::vector<std::string> result;
    size_t last = 0;
    size_t next = 0;
    while((next = s.find(delimiter, last)) != std::string::npos) {
        if(next > last) {
            result.emplace_back(s.substr(last, next - last));
        } else {
            result.emplace_back("");
        }
        last = next + delimiter.size();
    }
    if(last < s.size()) {
        result.emplace_back(s.substr(last));
    }
    return result;
}

std::vector<std::string> StringUtils::SplitStringWithFSM(const std::string &s, const char delimiter)
{
    enum
    {
        kStateInit = 0,
        kStateNormal = 1,
        kStateDelimiter = 2,
        kStateEnd = 3,
    };
    std::vector<std::string> result;
    int state = kStateInit;
    std::string tmp;
    state = kStateNormal;
    for(int pos = 0; pos < s.size();) {
        if(state == kStateNormal) {
            if(s.at(pos) == delimiter) {
                state = kStateDelimiter;
                continue;
            }
            tmp.push_back(s.at(pos));
            pos++;
        } else if(state == kStateDelimiter) {
            result.push_back(tmp);
            tmp.clear();
            state = kStateNormal;
            pos++;
        }
    }
    if(!tmp.empty()) {
        result.push_back(tmp);
    }
    return result;
}