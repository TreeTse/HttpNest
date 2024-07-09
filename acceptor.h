#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <functional>
#include <memory>
#include "event_loop.h"
#include "socket.h"
#include "log/log.h"

class Acceptor
{
public:
    Acceptor(EventLoop *loop, const std::string &ip, const uint16_t port);

    ~Acceptor();

    void NewConnection();

    void SetNewConnectionCallback(std::function<void(std::unique_ptr<Socket>)> cb);

private:
    EventLoop *loop_;
    Socket servsock_;
    Channel acceptChannel_;
    std::function<void(std::unique_ptr<Socket>)> newConnectionCb_;
};



#endif