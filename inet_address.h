#ifndef INET_ADDRESS_H
#define INET_ADDRESS_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

class InetAddress
{
public:
    InetAddress();

    InetAddress(const std::string &ip, uint16_t port);

    InetAddress(const sockaddr_in addr);

    ~InetAddress();

    const sockaddr *addr() const;

    const char *ip() const;

    uint16_t port() const;

    void SetAddr(sockaddr_in clientaddr);

private:
    sockaddr_in addr_;
};


#endif