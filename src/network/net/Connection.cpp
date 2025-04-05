#include "Connection.h"

using namespace nest::net;

Connection::Connection(EventLoop *loop, int fd, const InetAddress &localAddr, const InetAddress &peerAddr)
: Channel(loop, fd), localAddr_(localAddr), peerAddr_(peerAddr)
{}

void Connection::SetLocalAddr(const InetAddress &local)
{
    localAddr_ = local;
}

void Connection::SetPeerAddr(const InetAddress &peer)
{
    peerAddr_ = peer;
}

const InetAddress &Connection::LocalAddr() const
{
    return localAddr_;
}

const InetAddress &Connection::PeerAddr() const
{
    return peerAddr_;
}

void Connection::SetContext(int type, const std::shared_ptr<void> &context)
{
    contexts_[type] = context;
}

void Connection::SetContext(int type, std::shared_ptr<void> &&context)
{
    contexts_[type] = std::move(context);
}

void Connection::ClearContext(int type)
{
    contexts_[type].reset();
}

void Connection::ClearContext()
{
    contexts_.clear();
}

void Connection::SetActiveCallback(const ActiveCallback &cb)
{
    activeCb_ = cb;
}

void Connection::SetActiveCallback(ActiveCallback &&cb)
{
    activeCb_ = std::move(cb);
}

void Connection::Active()
{
    if (!active_.load()) {
        loop_->RunInLoop([this](){
            active_.store(true);
            if (activeCb_) {
                activeCb_(std::dynamic_pointer_cast<Connection>(shared_from_this()));
            }
        });
    }
}

void Connection::Deactive()
{
    active_.store(false);
}
