#include "http_conn.h"
//#include <fstream>

std::atomic<int> HttpConn::userCount_;
const char *HttpConn::srcDir_;

bool HttpConn::IsKeepAlive() const
{
    return request_.IsKeepAlive();
}

bool HttpConn::Process(Buffer &inputBuffer, Buffer &outputBuffer)
{
    if (request_.State() == HttpRequest::CHECK_STATE_FINISH) {
        request_.Init();
    }
    if (inputBuffer.ReadableBytes() <= 0) {
        return false;
    }
    HttpRequest::HttpCode status = request_.Parse(inputBuffer);
    if (status == HttpRequest::GET_REQUEST) {
        LOG_DEBUG("request path %s", request_.Path().c_str());
        response_.Init(srcDir_, request_.Path(), request_.IsKeepAlive(), 200);
    } else if (status == HttpRequest::NO_REQUEST) {
        return false;
    } else if (status == HttpRequest::INTERNAL_ERROR) {
        response_.Init(srcDir_, request_.Path(), false, 500);
    } else {
        response_.Init(srcDir_, request_.Path(), false, 400);
    }
    response_.MakeResponse(outputBuffer);
    return true;
}
