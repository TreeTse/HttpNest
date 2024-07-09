#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    ~HttpResponse();

    void Init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);

    void MakeResponse(Buffer &buff);

private:
    void AddStateLine(Buffer &buff);

    void AddHeader(Buffer &buff);

    void AddContent(Buffer &buff);

    std::string GetFileType();

    void ErrorContent(Buffer &buff, std::string message);

    int code_;
    bool isKeepAlive_;
    std::string path_;
    std::string srcDir_;
    struct stat mmFileStat_;
    static const std::unordered_map<int, std::string> CodeStatus_;
    static const std::unordered_map<std::string, std::string> SuffixType_;
};

#endif