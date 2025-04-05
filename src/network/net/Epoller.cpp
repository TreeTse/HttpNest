#include "Epoller.h"
#include "base/LogStream.h"
#include <string.h>

using namespace nest::net;

Epoller::Epoller(int maxEvent): events_(maxEvent)
{
    if ((fd_ = ::epoll_create(1024)) == -1) {
        LOG_ERROR << "epoll create failed.";
        exit(-1);
    }
}

Epoller::~Epoller()
{
    if (fd_ > 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

std::vector<ChannelPtr> Epoller::Poll(int timeoutMs)
{
    std::vector<ChannelPtr> channels;

    memset(&events_[0], 0x00, sizeof(struct epoll_event)*events_.size());
    auto ret = ::epoll_wait(fd_, (struct epoll_event*)&events_[0], static_cast<int>(events_.size()), timeoutMs);
    if (ret < 0) {
        LOG_ERROR << "epoll wait error. error:" << errno;
        exit(-1);
    }
    for (int i = 0; i < ret; i++) {
        struct epoll_event &ev = events_[i];
        if (ev.data.fd < 0) {
            continue;
        }
        auto iter = channels_.find(ev.data.fd);
        if (iter == channels_.end()) {
            continue;
        }

        ChannelPtr &ch = iter->second;
        ch->SetRevents(events_[i].events);
        channels.push_back(ch);
    }
    if (ret == events_.size()) {
        events_.resize(events_.size() * 2);
    }

    return channels;
}

void Epoller::AddChannel(const ChannelPtr &ch)
{
    auto iter = channels_.find(ch->fd());
    if (iter != channels_.end()) {
        return;
    }
    channels_[ch->fd()] = ch;

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.data.fd = ch->fd();
    ev.events = ch->Event();
    if (epoll_ctl(fd_, EPOLL_CTL_ADD, ch->fd(), &ev) == -1) {
        perror("epoll_ctl failed");
        exit(-1);
    }
}

void Epoller::RemoveChannel(const ChannelPtr &ch)
{
    auto iter = channels_.find(ch->fd());
    if (iter == channels_.end()) {
        return;
    }
    channels_.erase(iter);

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.data.fd = ch->fd();
    ev.events = ch->Event();
    if (epoll_ctl(fd_, EPOLL_CTL_DEL, ch->fd(), &ev) == -1) {
        perror("epoll_ctl failed");
        exit(-1);
    }
}

void Epoller::UpdateChannel(const ChannelPtr &ch)
{
    auto iter = channels_.find(ch->fd());
    if(iter == channels_.end()) {
        LOG_ERROR << "Can't find event fd:" << ch->fd();
        return;
    }

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.data.fd = ch->fd();
    ev.events = ch->Event();
    if (epoll_ctl(fd_, EPOLL_CTL_MOD, ch->fd(), &ev) == -1) {
        perror("epoll_ctl failed");
        exit(-1);
    }
}