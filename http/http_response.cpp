#include "http_response.h"

const std::unordered_map<int, std::string> HttpResponse::CodeStatus_ = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Error"},
};

const std::unordered_map<std::string, std::string> HttpResponse::SuffixType_ = {
        {".html",  "text/html"},
        {".xml",   "text/xml"},
        {".xhtml", "application/xhtml+xml"},
        {".txt",   "text/plain"},
        {".rtf",   "application/rtf"},
        {".pdf",   "application/pdf"},
        {".word",  "application/msword"},
        {".png",   "image/png"},
        {".gif",   "image/gif"},
        {".jpg",   "image/jpeg"},
        {".jpeg",  "image/jpeg"},
        {".au",    "audio/basic"},
        {".mpeg",  "video/mpeg"},
        {".mpg",   "video/mpeg"},
        {".avi",   "video/x-msvideo"},
        {".gz",    "application/x-gzip"},
        {".tar",   "application/x-tar"},
        {".css",   "text/css "},
        {".js",    "text/javascript "},
};

HttpResponse::~HttpResponse()
{}

void HttpResponse::Init(const std::string &srcDir, std::string &path, bool isKeepAlive, int code)
{
    assert(srcDir != "");
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    mmFileStat_ = {0};
}

std::string HttpResponse::GetFileType()
{
    std::string::size_type idx = path_.find_last_of('.');
    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if (SuffixType_.count(suffix) == 1) {
        return SuffixType_.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer &buff, std::string message)
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CodeStatus_.count(code_) == 1) {
        status = CodeStatus_.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServer</em></body></html>";

    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}

void HttpResponse::AddStateLine(Buffer &buff)
{
    std::string status;
    if (CodeStatus_.count(code_) == 1) {
        status = CodeStatus_.find(code_)->second;
    } else {
        code_ = 400;
        status = CodeStatus_.find(400)->second;
    }
    buff.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader(Buffer &buff)
{
    buff.Append("Connection: ");
    if (isKeepAlive_) {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer &buff)
{
    if (code_ != 200) {
        std::string status;
        if (CodeStatus_.count(code_) == 1) {
            status = CodeStatus_.find(code_)->second;
        } else {
            code_ = 400;
            status = CodeStatus_.find(400)->second;
        }
        ErrorContent(buff, status);
        return;
    }
    if (path_ == "/req_counter") {
        /* code */
    }
    
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if (srcFd < 0) {
        ErrorContent(buff, "File NotFound!");
        return;
    }
    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    //int *mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    char *mmRet = (char *)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        return;
    }

    close(srcFd);

    buff.Append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");

    buff.Append(mmRet, mmFileStat_.st_size);

    int re = munmap(mmRet, mmFileStat_.st_size);
    if (re == -1) {
        LOG_ERROR("munmap failed!");
    }
}

void HttpResponse::MakeResponse(Buffer &buff)
{
    if (code_ == 200 && path_ != "/req_counter") {
        if (stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
            code_ = 404;
        } else if (!(mmFileStat_.st_mode & S_IROTH)) {
            code_ = 403;
        }
    }
    AddStateLine(buff);
    AddHeader(buff);
    AddContent(buff);
}