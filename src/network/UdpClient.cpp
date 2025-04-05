#include "UdpClient.h"
#include "network/base/Socket.h"

using namespace nest::net;

UdpClient::UdpClient(EventLoop *loop, const InetAddress &server)
: UdpSocket(loop, -1, InetAddress(), server), servAddr_(server)
{}

UdpClient::~UdpClient()
{}

void UdpClient::Connect()
{
    loop_->RunInLoop([this](){
        ConnectInLoop();
    });
}

void UdpClient::ConnectInLoop()
{
    loop_->AssertInLoopThread();
    fd_ = Socket::CreateNonblockingUdpSocket(AF_INET);
    if(fd_ < 0) {
        OnClose();
        return;
    }
    connected_ = true;
    loop_->AddChannel(std::dynamic_pointer_cast<UdpClient>(shared_from_this()));
    Socket sock(fd_);
    sock.Connect(servAddr_);
    servAddr_.GetSockAddr((struct sockaddr*)&sockAddr_);
    if(connectedCb_) {
        connectedCb_(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()), true);
    }
}

void UdpClient::SetConnectedCallback(const ConnectedCallback &cb)
{
    connectedCb_ = cb;
}

void UdpClient::SetConnectedCallback(ConnectedCallback &&cb)
{
    connectedCb_ = std::move(cb);
}

void UdpClient::Send(std::list<BufferNodePtr> &list)
{}

void UdpClient::Send(const char *buf, size_t size)
{
    UdpSocket::Send(buf, size, (struct sockaddr*)&sockAddr_, sockLen_);
}

void UdpClient::OnClose()
{
    if(connected_) {
        loop_->DelChannel(std::dynamic_pointer_cast<UdpClient>(shared_from_this()));
        connected_ = false;
        UdpSocket::OnClose();
    }
}