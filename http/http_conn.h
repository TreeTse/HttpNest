#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <atomic>
#include "http_request.h"
#include "http_response.h"
#include "../buffer.h"

class HttpConn
{
public:
    HttpConn() {}
    ~HttpConn() = default;

public:
    bool IsKeepAlive() const;
    bool Process(Buffer &inputBuffer, Buffer &outputBuffer);

public:
    static std::atomic<int> userCount_;
    static const char *srcDir_;

private:
    bool isCounter_;
    struct stat fileStat_;

    HttpRequest request_;
    HttpResponse response_;
};

#endif
