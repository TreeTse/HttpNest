#include "tcpserver.h"


TcpServer::TcpServer(const std::string &ip, const uint16_t port, int threadNum)
            :threadNum_(threadNum), mainLoop_(new EventLoop(true)), acceptor_(mainLoop_.get(), ip, port), threadpool_(threadNum_, "IO")
{
    mainLoop_->SetEpollTimeoutCallback(std::bind(&TcpServer::EpollTimeout, this, std::placeholders::_1));

    acceptor_.SetNewConnectionCallback(std::bind(&TcpServer::NewConnection, this, std::placeholders::_1));

    for (int i = 0; i < threadNum_; i++) {
        subLoops_.emplace_back(new EventLoop(false, 5, 60));
        subLoops_[i]->SetEpollTimeoutCallback(std::bind(&TcpServer::EpollTimeout, this, std::placeholders::_1));
        subLoops_[i]->SetTimeoutCallback(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
        threadpool_.AddTask(std::bind(&EventLoop::Run, subLoops_[i].get()));
    }
}

TcpServer::~TcpServer()
{}

void TcpServer::Start()
{
    mainLoop_->Run();
}

void TcpServer::Stop()
{
    mainLoop_->Stop();
    LOG_DEBUG("main loop stopped.");
    for (int i = 0; i < threadNum_; i++) {
        subLoops_[i]->Stop();
    }
    LOG_DEBUG("sub loops stopped.");

    threadpool_.Stop();
    LOG_DEBUG("io thread stopped.");
}

void TcpServer::NewConnection(std::unique_ptr<Socket> clientSock)
{
    spConnection conn(new Connection(subLoops_[clientSock->fd() % threadNum_].get(), std::move(clientSock)));
    conn->SetCloseCallback(std::bind(&TcpServer::CloseConnection, this, std::placeholders::_1));
    conn->SetErrorCallback(std::bind(&TcpServer::ErrorConnection, this, std::placeholders::_1));
    conn->SetOnMessageCallback(std::bind(&TcpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    conn->SetSendCompleteCallback(std::bind(&TcpServer::SendComplete, this, std::placeholders::_1));
    {
        std::lock_guard<std::mutex> gd(connMtx_);
        conns_[conn->fd()] = conn;
    }
    subLoops_[conn->fd() % threadNum_]->NewConnection(conn);

    if (newConnectionCallback_) {
        newConnectionCallback_(conn);
    }
}

void TcpServer::CloseConnection(spConnection conn)
{
    if (closeConnectionCallback_) {
        closeConnectionCallback_(conn);
    }

    LOG_DEBUG("client(fd=%d) disconnected\n", conn->fd());
    {
        std::lock_guard<std::mutex> gd(connMtx_);
        conns_.erase(conn->fd());
    }
}

void TcpServer::ErrorConnection(spConnection conn)
{
    if (errorConnectionCallback_) {
        errorConnectionCallback_(conn);
    }

    LOG_DEBUG("client(fd=%d) error\n", conn->fd());
    {
        std::lock_guard<std::mutex> gd(connMtx_);
        conns_.erase(conn->fd());
    }
}

void TcpServer::OnMessage(spConnection conn, Buffer &buf)
{
    if (onMessageCallback_) {
        onMessageCallback_(conn, buf);
    }
}

void TcpServer::EpollTimeout(EventLoop *loop)
{
    if (timeoutCallback_) {
        timeoutCallback_(loop);
    }
}

void TcpServer::SendComplete(spConnection conn)
{
    if (sendCompleteCallback_) {
        sendCompleteCallback_(conn);
    }
}

void TcpServer::RemoveConnection(int fd)
{
    LOG_INFO("TcpServer::RemoveConnection() thread is %d.\n",syscall(SYS_gettid));

    std::lock_guard<std::mutex> gd(connMtx_);
    conns_.erase(fd);

    if (removeConnectionCallback_) removeConnectionCallback_(fd);
}

void TcpServer::SetTimeoutCb(std::function<void(EventLoop*)> fun)
{
    timeoutCallback_ = fun;
}

void TcpServer::SetCloseConnectionCb(std::function<void(spConnection)> fun)
{
    closeConnectionCallback_ = fun;
}

void TcpServer::SetErrorConnectionCb(std::function<void(spConnection)> fun)
{
    errorConnectionCallback_ = fun;
}

void TcpServer::SetOnMessageCb(std::function<void(spConnection, Buffer &buf)> fun)
{
    onMessageCallback_ = fun;
}

void TcpServer::SetNewConnectionCb(std::function<void(spConnection)> fun)
{
    newConnectionCallback_ = fun;
}

void TcpServer::SetSendCompleteCb(std::function<void(spConnection)> fun)
{
    sendCompleteCallback_ = fun;
}

void TcpServer::SetRemoveConnectionCb(std::function<void(int)> fun)
{
    removeConnectionCallback_ = fun;
}