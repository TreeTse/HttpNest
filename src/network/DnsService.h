#pragma once
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include "network/base/InetAddress.h"
#include "base/Singleton.h"
#include "base/NoCopyable.h"

namespace nest
{
    namespace net
    {
        using InetAddressPtr = std::shared_ptr<InetAddress>;
        class DnsService:public base::NonCopyable
        {
        public:
            DnsService() = default;
            ~DnsService();

            void AddHost(const std::string &host);
            std::vector<InetAddressPtr> GetHostAddress(const std::string &host);
            InetAddressPtr GetHostAddress(const std::string &host, int index);
            void UpdateHost(const std::string &host, std::vector<InetAddressPtr> &list);
            std::unordered_map<std::string, std::vector<InetAddressPtr>> GetHosts();
            void SetDnsServiceParam(int32_t interval, int32_t sleep, int32_t retry);
            void Start();
            void Stop();
            void OnWork();

            static void GetHostInfo(const std::string &host, std::vector<InetAddressPtr> &list);

        private:
            std::mutex mtx_;
            std::unordered_map<std::string, std::vector<InetAddressPtr>> hostsInfo_;
            std::thread thread_;
            bool running_{false};
            int32_t retry_{3};
            int32_t sleep_{200};
            int32_t interval_{180*1000};
        };
        #define sDnsService nest::base::Singleton<nest::net::DnsService>::Instance()
    }
}