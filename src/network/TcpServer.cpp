#include "TcpServer.h"
#include "base/LogStream.h"

using namespace nest::net;
using namespace nest::base;

TcpServer::TcpServer(EventLoop *loop, const InetAddress &addr)
: loop_(loop), addr_(addr)
{
    acceptor_ = std::make_shared<Acceptor>(loop, addr);
}

TcpServer::~TcpServer()
{}

void TcpServer::SetNewConnectionCallback(const std::function<void(const TcpConnectionPtr &)> &cb)
{
    newConnectionCb_ = cb;
}

void TcpServer::SetNewConnectionCallback(std::function<void(const TcpConnectionPtr &)> &&cb)
{
    newConnectionCb_ = std::move(cb);
}

void TcpServer::SetDestroyConnectionCallback(const std::function<void(const TcpConnectionPtr&)> &cb)
{
    destroyConnectionCb_ = cb;
}

void TcpServer::SetDestroyConnectionCallback(std::function<void(const TcpConnectionPtr&)> &&cb)
{
    destroyConnectionCb_ = std::move(cb);
}

void TcpServer::SetActiveCallback(const ActiveCallback &cb)
{
    activeCb_ = cb;
}

void TcpServer::SetActiveCallback(ActiveCallback &&cb)
{
    activeCb_ = std::move(cb);
}

void TcpServer::SetWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    writeCompleteCb_ = cb;
}

void TcpServer::SetWriteCompleteCallback(WriteCompleteCallback &&cb)
{   
    writeCompleteCb_ = std::move(cb);
}

void TcpServer::SetMessageCallback(const MessageCallback &cb)
{
    messageCb_ = cb;
}

void TcpServer::SetMessageCallback(MessageCallback &&cb)
{
    messageCb_ = std::move(cb);
}

void TcpServer::OnAccet(int fd, const InetAddress &addr)
{
    LOG_TRACE << "new connection fd:" << fd << " host:" << addr.ToIpPort();
    TcpConnectionPtr con = std::make_shared<TcpConnection>(loop_, fd, addr_,addr);
    con->SetCloseCallback(std::bind(&TcpServer::OnConnectionClose, this, std::placeholders::_1));
    if(writeCompleteCb_) {
        con->SetWriteCompleteCallback(writeCompleteCb_);
    } if(activeCb_) {
        con->SetActiveCallback(activeCb_);
    }
    con->SetRecvMsgCallback(messageCb_);
    connections_.insert(con);
    loop_->AddChannel(con);
    con->EnableCheckIdleTimeout(30);
    if(newConnectionCb_) {
        newConnectionCb_(con);
    }
}

void TcpServer::OnConnectionClose(const TcpConnectionPtr &con)
{
    LOG_TRACE << "host:" << con->PeerAddr().ToIpPort() << " closed.";
    loop_->AssertInLoopThread();
    connections_.erase(con);
    loop_->DelChannel(con);
    if(destroyConnectionCb_) {
        destroyConnectionCb_(con);
    }
}

void TcpServer::Start()
{
    acceptor_->SetAcceptCallback(std::bind(&TcpServer::OnAccet, this, std::placeholders::_1, std::placeholders::_2));
    acceptor_->Start();
}

void TcpServer::Stop()
{
    acceptor_->Stop();
}