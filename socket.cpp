#include "socket.h"

int CreateNonBlocking()
{
    int listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listenfd < 0) {
        perror("listen socket create error.");
        exit(-1);
    }
    return listenfd;
}

Socket::Socket(int fd): fd_(fd)
{}

Socket::~Socket()
{
    close(fd_);
}

int Socket::fd() const
{
    return fd_;
}

std::string Socket::ip() const
{
    return ip_;
}

uint16_t Socket::port() const
{
    return port_;
}

void Socket::SetIpAndPort(const std::string &ip, uint16_t port)
{
    ip_ = ip;
    port_ = port;
}

void Socket::SetReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::SetTcpNodelay(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::SetReusePort(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::SetKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

void Socket::SetLinger(bool on)
{
    int optval = on ? 1 : 0;
    struct linger tmp = {optval, 1};
    setsockopt(fd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
}

void Socket::Listen(int backlog)
{
    if (listen(fd_, backlog) != 0) {
        perror("listen failed.");
        close(fd_);
        exit(-1);
    }
}

void Socket::Bind(const InetAddress &servaddr)
{
    if (bind(fd_, servaddr.addr(), sizeof(sockaddr)) < 0) {
        perror("bind failed.");
        close(fd_);
        exit(-1);
    }

    SetIpAndPort(servaddr.ip(), servaddr.port());
}

int Socket::Accept(InetAddress &clientaddr)
{
    sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(fd_, (sockaddr*)&peeraddr, &len, SOCK_NONBLOCK);

    clientaddr.SetAddr(peeraddr);

    return clientfd;
}