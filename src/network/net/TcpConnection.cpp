#include "TcpConnection.h"
#include <unistd.h>
#include <iostream>
#include "base/LogStream.h"


using namespace nest::net;
using namespace nest::base;

TcpConnection::TcpConnection(EventLoop *loop, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
: Connection(loop, sockfd, localAddr, peerAddr)
{}

TcpConnection::~TcpConnection()
{
    OnClose();
}

void TcpConnection::SetCloseCallback(const CloseConnectionCallback &cb)
{
    closeCb_ = cb;
}

void TcpConnection::SetCloseCallback(CloseConnectionCallback &&cb)
{
    closeCb_ = std::move(cb);
}

void TcpConnection::SetRecvMsgCallback(const MessageCallback &cb)
{
    messageCb_ = cb;
}

void TcpConnection::SetRecvMsgCallback(MessageCallback &&cb)
{
    messageCb_ = std::move(cb);
}

void TcpConnection::SetWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    writeCompleteCb_ = cb;
}

void TcpConnection::SetWriteCompleteCallback(WriteCompleteCallback &&cb)
{
    writeCompleteCb_ = std::move(cb);
}

void TcpConnection::SetTimeoutCallback(int timeout, const TimeoutCallback &cb)
{
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());
    loop_->RunAfter(timeout, [&cp,&cb](){
        cb(cp);
    });
}

void TcpConnection::SetTimeoutCallback(int timeout, TimeoutCallback &&cb)
{
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());
    loop_->RunAfter(timeout, [&cp,cb](){
        cb(cp);
    });
}

void TcpConnection::OnClose()
{
    loop_->AssertInLoopThread();
    if(!isClosed_) {
        isClosed_ = true;
        if(closeCb_) {
            closeCb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
        }
        Channel::Close();
    }
}

void TcpConnection::ForceClose()
{
    loop_->RunInLoop([this](){
        OnClose();
    });
}

void TcpConnection::OnRead()
{
    if (isClosed_) {
        LOG_TRACE << "host:" << peerAddr_.ToIpPort() << " had closed.";
        return;
    }
    ExtendLife();
    while (true) {
        int err = 0;
        auto ret = msgBuffer_.ReadFd(fd_, &err);
        if (ret > 0) {
            if (messageCb_) {
                messageCb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()), msgBuffer_);
            }
        } else if (ret == 0) {
            OnClose();
            break;
        } else {
            if(err != EINTR && err != EAGAIN && err != EWOULDBLOCK) {
                LOG_ERROR << "read err:" << err;
                OnClose();
            }
            break;
        } 
    } 
}

void TcpConnection::OnError(const std::string &msg)
{
    LOG_ERROR << "host:" << peerAddr_.ToIpPort() << " error msg:" << msg;
    OnClose();
}

void TcpConnection::OnWrite()
{
    if (isClosed_) {
        LOG_TRACE << "host:" << peerAddr_.ToIpPort() << " had closed.";
        return;
    }
    ExtendLife();
    if (!iovecList_.empty()) {
        while (true) {
            auto ret = ::writev(fd_, &iovecList_[0], iovecList_.size());
            if (ret >= 0) {
                while (ret > 0) {
                    if (iovecList_.front().iov_len > ret) {
                        iovecList_.front().iov_base = (char*)iovecList_.front().iov_base + ret;
                        iovecList_.front().iov_len -= ret;
                        break;
                    } else {
                        ret -= iovecList_.front().iov_len;
                        iovecList_.erase(iovecList_.begin());
                    }
                }
                if (iovecList_.empty()) {
                    EnableWriting(false);
                    if(writeCompleteCb_) {
                        writeCompleteCb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
                    }
                    return;
                }
            } else {
                if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                    LOG_ERROR << "host:" << peerAddr_.ToIpPort() << " write err:" << errno;
                    OnClose();
                    return;
                }
                break;
            }    
        }
    } else {
        EnableWriting(false);
        if(writeCompleteCb_) {
            writeCompleteCb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
        }
    }
}

void TcpConnection::Send(std::list<BufferNodePtr> &list)
{
    loop_->RunInLoop([this, &list](){
        SendInLoop(list);
    });
}
void TcpConnection::Send(const char *buf, size_t size)
{
    loop_->RunInLoop([this, buf, size](){
        SendInLoop(buf, size);
    });
}

void TcpConnection::SendInLoop(const char *buf, size_t size)
{
    if (isClosed_) {
        LOG_TRACE << "host:" << peerAddr_.ToIpPort() << " had closed.";
        return;
    }
    size_t len = 0;
    if (iovecList_.empty()) {
        len = ::write(fd_, buf, size);
        if (len < 0) {
            if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR << "host:" << peerAddr_.ToIpPort() << " write err:" << errno;
                OnClose();
                return;
            }
            len = 0;
        }
        size -= len;
        if (size == 0) {
            if (writeCompleteCb_) {
                writeCompleteCb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
            }
            return;
        }
    }
    if (size > 0) {
        struct iovec vec;
        vec.iov_base = (void*)(buf + len);
        vec.iov_len = size;

        iovecList_.push_back(vec);
        EnableWriting(true);
    }
}

void TcpConnection::SendInLoop(std::list<BufferNodePtr> &list)
{
    if(isClosed_) {
        LOG_TRACE << "host:" << peerAddr_.ToIpPort() << " had closed.";
        return;
    }
    for(auto &l:list) {
        struct iovec vec;
        vec.iov_base = (void*)l->addr;
        vec.iov_len = l->size;
        
        iovecList_.push_back(vec);
    }
    if(!iovecList_.empty()) {
        EnableWriting(true);
    }
}

void TcpConnection::OnTimeout()
{
    LOG_ERROR << "host:" << peerAddr_.ToIpPort() << " timeout and close it.";
    std::cout << "host:" << peerAddr_.ToIpPort() << " timeout and close it." << std::endl;
    OnClose();
}

void TcpConnection::EnableCheckIdleTimeout(int32_t maxTime)
{
    auto tp = std::make_shared<TimeoutEntry>(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
    maxIdleTime_ = maxTime;
    timeoutEntry_ = tp;
    loop_->InsertEntry(maxTime, tp);
}

void TcpConnection::ExtendLife()
{
    auto tp = timeoutEntry_.lock();
    if (tp) {
        loop_->InsertEntry(maxIdleTime_, tp);
    }
}