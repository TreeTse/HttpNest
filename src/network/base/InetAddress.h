#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <bits/socket.h>
#include <string>

namespace nest
{
    namespace net
    {
        class InetAddress
        {
        public:    
            InetAddress(const std::string &ip, uint16_t port, bool isIpv6 = false);
            InetAddress(const std::string &host, bool isIpv6 = false);
            InetAddress() = default;
            ~InetAddress() = default;

            void SetHost(const std::string &host);
            void SetAddr(const std::string &addr);
            void SetPort(uint16_t port);
            void SetIsIPV6(bool is_v6);

            const std::string &ip() const;
            uint32_t IPv4() const;
            std::string ToIpPort() const;
            uint16_t port() const;
            void GetSockAddr(struct sockaddr *saddr) const;

            bool IsIpV6() const;
            bool IsWanIp() const;
            bool IsLanIp() const;
            bool IsLoopbackIp() const;

            static void GetIpAndPort(const std::string &host, std::string &ip, std::string &port);

        private:
            uint32_t IPv4(const char *ip) const;

            std::string addr_;
            std::string port_;
            bool isIpv6_{false};
        };
    }
}