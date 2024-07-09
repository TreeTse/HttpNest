#include "http_request.h"


const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
        {"/log.html", 0},
        {"/welcome.html", 1}};

HttpRequest::HttpRequest()
{
    Init();
}

void HttpRequest::Init()
{
    method_ = path_ = version_ = body_ = "";
    state_ = CHECK_STATE_REQUESTLINE;
    isCounter_ = false;
    header_.clear();
    post_.clear();
}

bool HttpRequest::ParseRequestLine(const std::string &line)
{
    // ^:start of the line, $:end of the line, [^ ]:match non-spaces, ():string to be fetched
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern)) {
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = CHECK_STATE_HEADER;
        return true;
    }
    LOG_ERROR("requestLine error! %s", line.c_str());
    return false;
}

void HttpRequest::ParseHeader(const std::string& line)
{
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, pattern)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = CHECK_STATE_CONTENT;
    }
}

void HttpRequest::ParsePath()
{
    if (path_ == "/") {
        path_ = "/judge.html";
    } else if (path_ == "/req_counter") {
        isCounter_ = true;
    }
}

bool HttpRequest::ParseContent(const std::string& line)
{
    body_ = line;
    if (!ParsePost()) {
        return false;
    }
    state_ = CHECK_STATE_FINISH;
    LOG_DEBUG("body: %s, len: %d", line.c_str(), line.size());
    return true;
}

int HttpRequest::ConvertHex(char ch)
{
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return ch - '0';
}

void HttpRequest::ParseFromUrlencoded()
{
    if (body_.size() == 0) return;

    std::string tmp, key, value;
    int num = 0;
    int n = body_.size();

    for (int i = 0; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
            case '=':
                key = tmp;
                tmp.clear();
                break;
            case '+':
                tmp += ' ';
                break;
            case '%':
                num = ConvertHex(body_[i + 1]) * 16 + ConvertHex(body_[i + 2]);
                tmp += static_cast<char>(num);
                i += 2;
                break;
            case '&':
                value = tmp;
                tmp.clear();
                post_[key] = value;
                LOG_DEBUG("ParseFromUrlencoded: %s = %s", key.c_str(), value.c_str());
                break;
            default:
                tmp += ch;
                break;
        }
    }
    if (post_.count(key) == 0) {
        value = tmp;
        post_[key] = value;
    }
}

// Currently, only "application/x-www-form-urlencoded" can be parsed.
bool HttpRequest::ParsePost()
{
    if (body_.size() < atol(header_["Content-Length"].c_str())) {
        return false;
    }
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded();
        if (DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if (UserVerify(post_["user"], post_["password"], isLogin)) {
                    if (isLogin) {
                        path_ = "/welcome.html";
                    } else {
                        path_ = "/log.html";
                    }
                } else {
                    if (isLogin) {
                        path_ = "/logError.html";
                    } else {
                        path_ = "/registerError.html";
                    }
                } 
            }
        }
    }
    return true;
}

HttpRequest::CheckState HttpRequest::State() const
{
    return state_;
}

std::string& HttpRequest::Path()
{
    return path_;
}

bool HttpRequest::IsKeepAlive() const
{
    if (header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin)
{
    if (name == "" || pwd == "") {
        return false;
    }
    LOG_INFO("verify name:%s, pwd:%s", name.c_str(), pwd.c_str());

    bool valid = false;
    MYSQL *sql = nullptr;
    SqlConnRAII sqlConn(&sql, SqlConnPool::GetInstance());
    assert(sql);

    char order[256] = {0};
    MYSQL_RES *res = nullptr;

    if (isLogin) {
        valid = true;
    }

    snprintf(order, 256, "SELECT username,passwd FROM user WHERE username='%s' LIMIT 1", name.c_str());

    if (mysql_query(sql, order)) {
        mysql_free_result(res);
        LOG_ERROR("SELECT error:%s", mysql_error(sql));
        return false;
    }

    res = mysql_store_result(sql);

    while (MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("Mysql row: %s %s", row[0], row[1]);
        std::string password(row[1]);
        if (isLogin) {
            if (pwd == password) {
                valid = true;
            } else {
                LOG_DEBUG("Login error!");
                valid = false;
            }
        } else {
            LOG_DEBUG("User used!");
            valid = false;
        }
    }
    mysql_free_result(res);

    if (!isLogin && valid) {
        LOG_DEBUG("Register!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        if (mysql_query(sql, order)) {
            LOG_ERROR("INSERT error:%s", mysql_error(sql));
            valid = false;
        }
        valid = true;
    }

    LOG_DEBUG("UserVerify success!");
    return valid;
}

HttpRequest::HttpCode HttpRequest::Parse(Buffer &buf)
{
    const char CRLF[] = "\r\n";
    if (buf.ReadableBytes() <= 0) {
        return NO_REQUEST;
    }
    while (buf.ReadableBytes() && state_ != CHECK_STATE_FINISH) {
        const char *lineEnd = std::search(buf.Peek(), buf.BeginWriteConst(), CRLF, CRLF + 2);
        // if there is no CRLF at the end of the data, exiting the loop and waiting to receive the remaining data.
        if (lineEnd == buf.BeginWriteConst() && state_ == CHECK_STATE_HEADER) {
            break;
        }
        std::string line(buf.Peek(), lineEnd);
        LOG_DEBUG("HttpRequest::Parse line: %s", line.c_str());
        //std::cout << "HttpRequest::Parse line: " << line << std::endl;
        switch (state_) {
            case CHECK_STATE_REQUESTLINE:
                if (!ParseRequestLine(line)) {
                    return BAD_REQUEST;
                }
                ParsePath();
                break;
            case CHECK_STATE_HEADER:
                ParseHeader(line);
                if (state_ == CHECK_STATE_CONTENT && method_ == "GET") {
                    state_ = CHECK_STATE_FINISH;
                    buf.RetrieveAll();
                    return GET_REQUEST;
                }
                break;
            case CHECK_STATE_CONTENT:
                if (!ParseContent(line)) {
                    return NO_REQUEST;
                }
                buf.RetrieveAll();
                return GET_REQUEST;
            default:
                return INTERNAL_ERROR;
        }
        buf.RetrieveUntil(lineEnd + 2);
    }
    if (state_ == CHECK_STATE_CONTENT) {
        state_ = CHECK_STATE_FINISH;
        buf.RetrieveAll();
        return GET_REQUEST;
    }
    return NO_REQUEST;
}