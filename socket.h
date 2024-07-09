#ifndef SOCKET_H
#define SOCKET_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "inet_address.h"
#include "log/log.h"

int CreateNonBlocking();

class Socket
{
public:
    Socket(int fd);

    ~Socket();

    int fd() const;

    std::string ip() const;

    uint16_t port() const;

    void SetIpAndPort(const std::string &ip, uint16_t port);

    void SetReuseAddr(bool on);

    void SetTcpNodelay(bool on);

    void SetReusePort(bool on);

    void SetKeepAlive(bool on);

    void SetLinger(bool on);

    void Listen(int backlog=128);

    void Bind(const InetAddress &servaddr);

    int Accept(InetAddress &clientaddr);

private:
    const int fd_;
    std::string ip_;
    uint16_t port_;
};


#endif