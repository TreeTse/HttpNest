#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include "channel.h"

class Channel;

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);

    ~Epoller();

    void UpdateChannel(Channel *ch);

    void RemoveChannel(Channel *ch);

    std::vector<Channel *> Poll(int timeoutMs = -1);

private:
    int epollfd_ = -1;

    std::vector<struct epoll_event> events_;
};

#endif