#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <unordered_map>
#include <algorithm>
#include <regex>
#include "../buffer.h"
#include "../log/log.h"
#include "../mysql/sql_connection_pool.h"

class HttpRequest {
public:
    enum HttpCode
    {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        COUNTER_REQUEST,//req_counter
        CLOSED_CONNECTION
    };

    enum CheckState
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT,
        CHECK_STATE_FINISH,
    };

    HttpRequest();
    ~HttpRequest() {}

    void Init();
    HttpCode Parse(Buffer &buf);

    CheckState State() const;

    std::string& Path();

    bool IsKeepAlive() const;

private:
    bool ParseRequestLine(const std::string& line);

    void ParsePath();

    void ParseHeader(const std::string& line);

    bool ParseContent(const std::string& line);

    bool ParsePost();

    void ParseFromUrlencoded();

    static int ConvertHex(char ch);

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);

    std::string method_, path_, version_, body_;
    CheckState state_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    bool isCounter_;
};

#endif