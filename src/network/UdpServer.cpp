#include "UdpServer.h"

using namespace nest::net;

UdpServer::UdpServer(EventLoop *loop, const InetAddress &server)
: UdpSocket(loop, -1, server, InetAddress()), server_(server)
{}

UdpServer::~UdpServer()
{
    Stop();
}

void UdpServer::Start()
{
    loop_->RunInLoop([this](){
        Open();
    });
}

void UdpServer::Stop()
{
    loop_->RunInLoop([this](){
        loop_->DelChannel(std::dynamic_pointer_cast<UdpServer>(shared_from_this()));
        OnClose();
    });
}

void UdpServer::Open()
{
    loop_->AssertInLoopThread();
    fd_ = Socket::CreateNonblockingUdpSocket(AF_INET);
    if(fd_ < 0) {
        OnClose();
        return;
    }
    loop_->AddChannel(std::dynamic_pointer_cast<UdpServer>(shared_from_this()));
    Socket sock(fd_);
    sock.BindAddress(server_);
}