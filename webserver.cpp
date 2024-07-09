#include "webserver.h"

WebServer::WebServer(const std::string &ip, const uint16_t port, int timeoutMs, bool optLinger, const char *sqlUser,
                     const char *sqlPwd, const char *dbName, int connNum, int subThreadNum, int workThreadNum)
                :tcpserver_(ip, port, subThreadNum), threadpool_(workThreadNum, "WORK"), timeoutMS_(timeoutMs), optLinger_(optLinger), isClose_(false)
{
    srcDir_ = getcwd(nullptr, 200);
    assert(srcDir_);
    strcat(srcDir_, "/root");

    HttpConn::userCount_ = 0;
    HttpConn::srcDir_ = srcDir_;

    SqlConnPool::GetInstance()->Init("localhost", sqlUser, sqlPwd, dbName, 3306, connNum);

    tcpserver_.SetNewConnectionCb(std::bind(&WebServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.SetCloseConnectionCb(std::bind(&WebServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.SetErrorConnectionCb(std::bind(&WebServer::HandleError, this, std::placeholders::_1));
    tcpserver_.SetOnMessageCb(std::bind(&WebServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.SetSendCompleteCb(std::bind(&WebServer::HandleSendComplete, this, std::placeholders::_1));

    if (isClose_) {
        LOG_ERROR("================= Server init error! ===================");
    } else {
        LOG_INFO("============= Server init ================");
        LOG_INFO("srcDir: %s", srcDir_);
    }
}

WebServer::~WebServer()
{
    isClose_ = true;
    free(srcDir_);
    //SqlConnPool::GetInstance()->DestroyPool();
    //delete acceptor_;
    //delete mainLoop_;
    /*for (auto &conn : conns_) {
        delete conn.second;
    }*/
    //for (auto &loop : subLoops_) {
        //delete loop;
    //}
    
}

void WebServer::HandleNewConnection(spConnection conn)
{
    LOG_DEBUG("%s new connection(fd=%d,ip=%s,port=%d) ok.", TimeStamp::Now().Tostring().c_str(), conn->fd(), conn->ip().c_str(), conn->port());
}

void WebServer::HandleClose(spConnection conn)
{
    LOG_DEBUG("%s connection closed(fd=%d,ip=%s,port=%d).", TimeStamp::Now().Tostring().c_str(), conn->fd(), conn->ip().c_str(), conn->port());
}

void WebServer::HandleError(spConnection conn)
{
    LOG_DEBUG("handle error, fd:%d", conn->fd());
}

void WebServer::HandleMessage(spConnection conn, Buffer &buf)
{
    if (threadpool_.GetSize() == 0) {
        OnMessage(conn, buf);
    } else {
        threadpool_.AddTask(std::bind(&WebServer::OnMessage, this, conn, buf));
    }
}

void WebServer::OnMessage(spConnection conn, Buffer &buf)
{
    LOG_DEBUG("on message, fd:%d", conn->fd());
    /*int len;
    memcpy(&len, buf->Peek(), 4);
    buf->Retrieve(4);
    message = buf->RetrieveAsStr(len);*/
    //std::cout << "on mess " << message << std::endl;
    //printf("%s message(fd=%d) is: %s\n", TimeStamp::Now().Tostring().c_str(), conn->fd(), message.c_str());

    Buffer outputBuffer;
    HttpConn http;
    http.Process(buf, outputBuffer);

    std::string message = outputBuffer.RetrieveAllToStr();
    conn->Send(message.data(), message.size());
}

void WebServer::HandleSendComplete(spConnection conn)
{
    LOG_DEBUG("Send Complete, fd:%d", conn->fd());
}

void WebServer::Start()
{
    bool stopServer = false;
    int timeMs = -1;
    if (!isClose_) {
        LOG_INFO("================Server start================");
    }
    tcpserver_.Start();
}

void WebServer::Stop()
{
    threadpool_.Stop();
    LOG_DEBUG("work thread stopped.");

    tcpserver_.Stop();
    LOG_DEBUG("tcpserver stopped.");
}

int WebServer::SetFdNonBlocking(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
}