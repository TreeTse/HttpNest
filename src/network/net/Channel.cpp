#include "Channel.h"
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include "EventLoop.h"

using namespace nest::net;

Channel::Channel(EventLoop *loop)
: loop_(loop)
{}

Channel::Channel(EventLoop *loop, int fd)
: loop_(loop), fd_(fd)
{}

Channel::~Channel()
{
    Close();
}

void Channel::Close()
{
    if (fd_ > 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool Channel::EnableWriting(bool enable)
{
    return loop_->EnableChannelWriting(shared_from_this(), enable);
}

bool Channel::EnableReading(bool enable)
{
    return loop_->EnableChannelReading(shared_from_this(), enable);
}

int Channel::fd() const
{
    return fd_;
}

void Channel::SetRevents(uint32_t ev)
{
    revents_ = ev;
}

uint32_t Channel::Revents()
{
    return revents_;
}

void Channel::HandleEvent()
{
    if (revents_ & EPOLLERR) {
        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &len);

        OnError(strerror(error));
    } else if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        OnClose();
    } else if (revents_ & (EPOLLIN | EPOLLPRI)) {
        OnRead();
    } else if (revents_ & EPOLLOUT) {
        OnWrite();
    }
}

uint32_t Channel::Event()
{
    return event_;
}