#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "tcpserver.h"
#include "./http/http_conn.h"


class WebServer
{
public:
    WebServer(const std::string &ip, const uint16_t port, int timeoutMs, bool optLinger, const char* sqlUser, const char* sqlPwd,
              const char *dbName, int connNum, int subThreadNum, int workThreadNum);
    ~WebServer();
    void Start();
    void Stop();
    static int SetFdNonBlocking(int fd);

private:
    void HandleNewConnection(spConnection conn);
    void HandleClose(spConnection conn);
    void HandleError(spConnection conn);
    void HandleMessage(spConnection conn, Buffer &buf);
    void HandleSendComplete(spConnection conn);
    void OnMessage(spConnection conn, Buffer &buf);

public:
    char *srcDir_;
    bool optLinger_;

private:
    int timeoutMS_;
    bool isClose_;
    TcpServer tcpserver_;
    ThreadPool threadpool_;
};
#endif
