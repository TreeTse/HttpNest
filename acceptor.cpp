#include "acceptor.h"

Acceptor::Acceptor(EventLoop *loop, const std::string &ip, const uint16_t port)
            :loop_(loop), servsock_(CreateNonBlocking()), acceptChannel_(loop_, servsock_.fd())
{
    InetAddress servaddr(ip, port);
    servsock_.SetReuseAddr(true);
    servsock_.SetTcpNodelay(true);
    //servsock_.SetLinger(true);
    servsock_.Bind(servaddr);
    servsock_.Listen();

    acceptChannel_.SetReadCallback(std::bind(&Acceptor::NewConnection, this));
    acceptChannel_.EnableReading();
}


Acceptor::~Acceptor()
{}

void Acceptor::NewConnection()
{
    InetAddress clientAddr;
    std::unique_ptr<Socket> clientSock(new Socket(servsock_.Accept(clientAddr)));
    clientSock->SetIpAndPort(clientAddr.ip(), clientAddr.port());

    LOG_DEBUG("Accept client(fd=%d,ip=%s,port=%d) ok.", clientSock->fd(), clientAddr.ip(), clientAddr.port())

    newConnectionCb_(std::move(clientSock));
}

void Acceptor::SetNewConnectionCallback(std::function<void(std::unique_ptr<Socket>)> cb)
{
    newConnectionCb_ = cb;
}