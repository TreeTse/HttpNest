#include "UdpSocket.h"
#include "base/LogStream.h"

using namespace nest::net;
using namespace nest::base;

UdpSocket::UdpSocket(EventLoop *loop, 
                     int socketfd, 
                     const InetAddress &localAddr, 
                     const InetAddress &peerAddr)
: Connection(loop, socketfd, localAddr, peerAddr), messageBuffer_(messageBufferSize_)
{}

UdpSocket::~UdpSocket()
{}

void UdpSocket::SetCloseCallback(const UdpSocketCloseConnectionCallback &cb)
{
    closeCb_ = cb;
}

void UdpSocket::SetCloseCallback(UdpSocketCloseConnectionCallback &&cb)
{
    closeCb_ = std::move(cb);
}

void UdpSocket::SetRecvMsgCallback(const UdpSocketMessageCallback &cb)
{
    messageCb_ = cb;
}

void UdpSocket::SetRecvMsgCallback(UdpSocketMessageCallback &&cb)
{
    messageCb_ = std::move(cb);
}

void UdpSocket::SetWriteCompleteCallback(const UdpSocketWriteCompleteCallback &cb)
{
    writeCompleteCb_ = cb;
}

void UdpSocket::SetWriteCompleteCallback(UdpSocketWriteCompleteCallback &&cb)
{
    writeCompleteCb_ = std::move(cb);
}

void UdpSocket::SetTimeoutCallback(int timeout, const UdpSocketTimeoutCallback &cb)
{
    auto us = std::dynamic_pointer_cast<UdpSocket>(shared_from_this());
    loop_->RunAfter(timeout, [cb,us](){
        cb(us);
    });
}

void UdpSocket::SetTimeoutCallback(int timeout, UdpSocketTimeoutCallback &&cb)
{
    auto us = std::dynamic_pointer_cast<UdpSocket>(shared_from_this());
    loop_->RunAfter(timeout, [cb,us](){
        cb(us);
    });
}

void UdpSocket::OnTimeOut()
{
    LOG_TRACE << "host:" << peerAddr_.ToIpPort() << " timeout and close it.";
    OnClose();
}

void UdpSocket::OnError(const std::string &msg)
{
    LOG_TRACE << "host:" << peerAddr_.ToIpPort() << " error:" << msg;
    OnClose();
}

void UdpSocket::OnRead()
{
    if (isClosed_) {
        LOG_WARN << "host: " << peerAddr_.ToIpPort() << " has closed.";
        return;
    }
    ExtendLife();
    while (true) {
        struct sockaddr_in6 sock_addr;
        socklen_t len = sizeof(sock_addr);
        auto ret = ::recvfrom(fd_, messageBuffer_.BeginWrite(), messageBufferSize_, 0,
                (struct sockaddr *)&sock_addr, &len);
        if (ret > 0) {
            InetAddress peeraddr;
            messageBuffer_.HasWritten(ret);

            if(sock_addr.sin6_family == AF_INET) {
                char ip[16] = {0,};
                struct sockaddr_in *saddr = (struct sockaddr_in*)&sock_addr;
                ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
                peeraddr.SetAddr(ip);
                peeraddr.SetPort(ntohs(saddr->sin_port));
            } else if(sock_addr.sin6_family == AF_INET6) {
                char ip[INET6_ADDRSTRLEN] = {0,};
                ::inet_ntop(AF_INET6, &(sock_addr.sin6_addr), ip, sizeof(ip));
                peeraddr.SetAddr(ip);
                peeraddr.SetPort(ntohs(sock_addr.sin6_port));
                peeraddr.SetIsIPV6(true);
            }
            if (messageCb_) {
                messageCb_(peeraddr, messageBuffer_);
            }
            messageBuffer_.RetrieveAll();
        } else if (ret < 0) {
            if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR << "host:" << peerAddr_.ToIpPort() << " error:" << errno;
                OnClose();
                return;
            }
            break;
        }     
    }   
}

void UdpSocket::OnWrite()
{
    if (isClosed_) {
        LOG_WARN << "host:" << peerAddr_.ToIpPort() << " had closed.";
        return;
    }
    ExtendLife();
    while (true) {
        if (!bufferList_.empty()) {
            auto buf = bufferList_.front();
            auto ret = ::sendto(fd_, buf->addr, buf->size, 0, buf->sock_addr, buf->sock_len);
            if (ret > 0) {
                bufferList_.pop_front();
            } else if (ret < 0) {
                if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                    LOG_ERROR << "host:" << peerAddr_.ToIpPort() << " error:" << errno;
                    OnClose();
                    return;
                }
                break;
            }
        }
        if (bufferList_.empty()) {
            if (writeCompleteCb_) {
                writeCompleteCb_(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
            }
            break;
        }
    }
}

void UdpSocket::OnClose()
{
    if (!isClosed_) {
        isClosed_ = true;
        if (closeCb_) {
            closeCb_(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
        }
        Channel::Close();
    }
}

void UdpSocket::EnableCheckIdleTimeout(int32_t maxTime)
{
    auto tp = std::make_shared<UdpTimeoutEntry>(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
    maxIdleTime_ = maxTime;
    timeoutEntry_ = tp;
    loop_->InsertEntry(maxTime, tp);
}

void UdpSocket::Send(std::list<UdpBufferNodePtr> &list)
{
    loop_->RunInLoop([this,&list](){
        SendInLoop(list);
    });
}

void UdpSocket::Send(const char *buf, size_t size, struct sockaddr *addr, socklen_t len)
{
    loop_->RunInLoop([this,buf,size,addr,len](){
        SendInLoop(buf,size,addr,len);
    });
}

void UdpSocket::ForceClose()
{
    loop_->RunInLoop([this](){
        OnClose();
    });
}

void UdpSocket::SendInLoop(std::list<UdpBufferNodePtr> &list)
{
    for(auto &i:list) {
        bufferList_.emplace_back(i);
    }
    if(!bufferList_.empty()) {
        EnableWriting(true);
    }
}

void UdpSocket::SendInLoop(const char *buf, size_t size, struct sockaddr*saddr, socklen_t len)
{
    if(bufferList_.empty()) {
        auto ret = ::sendto(fd_, buf, size, 0, saddr, len);
        if(ret > 0) {
            return;
        }
    }
    auto node = std::make_shared<UdpBufferNode>((void*)buf, size, saddr, len);
    bufferList_.emplace_back(node);
    EnableWriting(true);
}

void UdpSocket::ExtendLife()
{

}