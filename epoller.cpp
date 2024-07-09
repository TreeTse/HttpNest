#include "epoller.h"

Epoller::Epoller(int maxEvent): events_(maxEvent)
{
    assert(events_.size() > 0);
    if ((epollfd_ = epoll_create(1)) == -1) {
        perror("epoll_create failed");
        exit(-1);
    }
}

Epoller::~Epoller()
{
    close(epollfd_);
}

void Epoller::UpdateChannel(Channel *ch)
{
    epoll_event ev = {0};
    ev.data.ptr = ch;
    ev.events = ch->Events();

    if (ch->InEpoll()) {
        if (epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev) == -1) {
            perror("epoll_ctl failed");
            exit(-1);
        }
    } else {
        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev) == -1) {
            perror("epoll_ctl failed");
            exit(-1);
        }
        ch->SetInEpoll(true);
    }
}

void Epoller::RemoveChannel(Channel *ch)
{
    if (ch->InEpoll()) {
        if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), 0) == -1) {
            perror("epoll_ctl failed");
            exit(-1);
        }
    }
}

std::vector<Channel *> Epoller::Poll(int timeoutMs)
{
    std::vector<Channel *> channels;

    int eventCnt = epoll_wait(epollfd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);

    if (eventCnt < 0) {
        perror("epoll_wait failed");
        exit(-1);
    }
    if (eventCnt == 0) {
        return channels;
    }

    for (int i = 0; i < eventCnt; i++) {
        Channel *ch = (Channel *)events_[i].data.ptr;
        ch->SetRevents(events_[i].events);
        channels.push_back(ch);
    }

    return channels;
}