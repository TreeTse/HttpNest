#include "TcpClient.h"
#include "network/base/Socket.h"
#include "base/LogStream.h"

using namespace nest::net;

TcpClient::TcpClient(EventLoop *loop, const InetAddress &server)
:TcpConnection(loop, -1, InetAddress(), server), servAddr_(server)
{}

TcpClient::~TcpClient()
{
    OnClose();
}

void TcpClient::SetConnectCallback(const ConnectionCallback &cb)
{
    connectedCb_ = cb;
}

void TcpClient::SetConnectCallback(ConnectionCallback &&cb)
{
    connectedCb_ = std::move(cb);
}

void TcpClient::Connect()
{
    loop_->RunInLoop([this](){
        ConnectInLoop();
    });
}

void TcpClient::ConnectInLoop()
{
    loop_->AssertInLoopThread();
    fd_ = Socket::CreateNonblockingTcpSocket(AF_INET);
    if (fd_ < 0) {
        OnClose();
        return;
    }
    status_ = kTcpConStatusConnecting;
    loop_->AddChannel(std::dynamic_pointer_cast<TcpClient>(shared_from_this()));
    EnableWriting(true);
    EnableCheckIdleTimeout(3);
    Socket sock(fd_);
    auto ret = sock.Connect(servAddr_);
    if (ret == 0) {
        UpdateConnectionStatus();
        return;
    } else if(ret == -1) {
        if(errno != EINPROGRESS) {
            LOG_ERROR << "connect to server:" << servAddr_.ToIpPort() << " error:" << errno;
            OnClose();
            return;
        }
    }
}

void TcpClient::UpdateConnectionStatus()
{
    status_ = kTcpConStatusConnected;
    if(connectedCb_) {
        connectedCb_(std::dynamic_pointer_cast<TcpClient>(shared_from_this()), true);
    }
}

void TcpClient::OnRead()
{
    if(status_ == kTcpConStatusConnecting) {
        if(CheckError()) {
            LOG_ERROR << "connect to server:" << servAddr_.ToIpPort() << " error:" << errno;
            OnClose();
            return;
        }
        UpdateConnectionStatus();
        return;
    } else if(status_ == kTcpConStatusConnected) {
        TcpConnection::OnRead();
    }
}

void TcpClient::OnWrite()
{
    if(status_ == kTcpConStatusConnecting) {
        if(CheckError()) {
            LOG_ERROR << "connect to server:" << servAddr_.ToIpPort() << " error:" << errno;
            OnClose();
            return;
        }
        UpdateConnectionStatus();
        return;
    } else if(status_ == kTcpConStatusConnected) {
        TcpConnection::OnWrite();
    }
}

void TcpClient::OnClose()
{
    if(status_ == kTcpConStatusConnecting || status_ == kTcpConStatusConnected) {
        loop_->DelChannel(std::dynamic_pointer_cast<TcpClient>(shared_from_this()));
    }
    status_ = kTcpConStatusDisConnected;
    TcpConnection::OnClose();
}

bool TcpClient::CheckError()
{
    int error = 0;
    socklen_t len = sizeof(error);
    ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &len);
    return error != 0;
}

void TcpClient::Send(std::list<BufferNodePtr> &list)
{
    if (status_ == kTcpConStatusConnected) {
        TcpConnection::Send(list);
    }
}

void TcpClient::Send(const char *buf, size_t size)
{
    if(status_ == kTcpConStatusConnected) {
        TcpConnection::Send(buf, size);
    }
}