#include "channel.h"


Channel::Channel(EventLoop *loop, int fd): loop_(loop), fd_(fd)
{}

Channel::~Channel() // No need to destroy fd_ and loop_ here
{}

int Channel::fd()
{
    return fd_;
}

uint32_t Channel::Events()
{
    return events_;
}

void Channel::SetRevents(uint32_t ev)
{
    revents_ = ev;
}

bool Channel::InEpoll()
{
    return inEpoll_;
}

void Channel::SetInEpoll(bool inepoll)
{
    inEpoll_ = inepoll;
}

uint32_t Channel::Revents()
{
    return revents_;
}

void Channel::EnableET()
{
    events_ |= EPOLLET;
}

void Channel::EnableReading()
{
    events_ |= EPOLLIN;
    loop_->UpdateChannel(this);
}

void Channel::DisableReading()
{
    events_ &= ~EPOLLIN;
    loop_->UpdateChannel(this);
}

void Channel::EnableWriting()
{
    events_ |= EPOLLOUT;
    loop_->UpdateChannel(this);
}

void Channel::DisableWriting()
{
    events_ &= ~EPOLLOUT;
    loop_->UpdateChannel(this);
}

void Channel::DisableAll()
{
    events_ = 0;
    loop_->UpdateChannel(this);
}

void Channel::Remove()
{
    DisableAll();
    loop_->RemoveChannel(this);
}

void Channel::HandleEvent()
{
    if (revents_ & EPOLLRDHUP) {
        closeCallback_();
    } else if (revents_ & (EPOLLIN | EPOLLPRI)) {
        readCallback_();
    } else if (revents_ & EPOLLOUT) {
        writeCallback_();
    } else {
        errorCallback_();
    }
}

void Channel::SetReadCallback(std::function<void()> cb)
{
    readCallback_ = cb;
}

void Channel::SetCloseCallback(std::function<void()> cb)
{
    closeCallback_ = cb;
}

void Channel::SetErrorCallback(std::function<void()> cb)
{
    errorCallback_ = cb;
}

void Channel::SetWriteCallback(std::function<void()> cb)
{
    writeCallback_ = cb;
}