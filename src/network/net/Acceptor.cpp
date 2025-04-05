#include "Acceptor.h"
#include "base/LogStream.h"

using namespace nest::net;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &addr)
: Channel(loop), addr_(addr)
{}

Acceptor::~Acceptor()
{
    Stop();
    if(socketOpt_) {
        delete socketOpt_;
        socketOpt_ = nullptr;
    }      
}

void Acceptor::SetAcceptCallback(const AcceptCallback &cb)
{
    acceptCb_ = cb;
}

void Acceptor::SetAcceptCallback(AcceptCallback &&cb)
{
    acceptCb_ = std::move(cb);
}

void Acceptor::Open()
{
    if(fd_ > 0) {
        ::close(fd_);
        fd_ = -1;
    }
    if(addr_.IsIpV6()) {
        fd_ = Socket::CreateNonblockingTcpSocket(AF_INET6);
    } else {
        fd_ = Socket::CreateNonblockingTcpSocket(AF_INET);
    }
    if(fd_ < 0) {
        LOG_ERROR << "socket failed.errno:" << errno;
        exit(-1);
    }
    if(socketOpt_) {
        delete socketOpt_;
        socketOpt_ = nullptr;
    }
    loop_->AddChannel(std::dynamic_pointer_cast<Acceptor>(shared_from_this()));
    socketOpt_ = new Socket(fd_);
    socketOpt_->SetReuseAddr(true);
    socketOpt_->SetReusePort(true);    
    socketOpt_->BindAddress(addr_);
    socketOpt_->Listen();
}

void Acceptor::Start()
{
    loop_->RunInLoop([this](){
        Open();
    });
}

void Acceptor::Stop()
{
    loop_->DelChannel(std::dynamic_pointer_cast<Acceptor>(shared_from_this()));
}

void Acceptor::OnRead()
{
    if(!socketOpt_) {
        return;
    }
    while(true) {
        InetAddress addr;
        auto sock = socketOpt_->Accept(&addr);
        if(sock >= 0) {
            if(acceptCb_) {
                acceptCb_(sock, addr);
            }
        } else {
            if(errno != EINTR && errno != EAGAIN) {
                LOG_ERROR << "acceptor error, errno:" << errno;
                OnClose();
            }
            break;
        }
    }
}

void Acceptor::OnError(const std::string &msg)
{
    LOG_ERROR << "acceptor error:" << msg;
    OnClose();
}

void Acceptor::OnClose()
{
    Stop();
    Open();
}