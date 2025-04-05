#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include "json/json.h"
#include "NoCopyable.h"
#include "Singleton.h"
#include "Logger.h"

namespace nest
{
    namespace base
    {
        struct LogInfo
        {
            LogLevel level;
            std::string path;
            std::string name;
            RotateType rotate_type{kRotateNone};
        };
        using LogInfoPtr = std::shared_ptr<LogInfo>;

        struct ServiceInfo
        {
            std::string addr;
            uint16_t port;
            std::string protocol;
            std::string transport;
        };
        using ServiceInfoPtr = std::shared_ptr<ServiceInfo>;

        class Config
        {
        public:
            Config() = default;
            ~Config() = default;

            bool LoadConfig(const std::string &file);
            LogInfoPtr& GetLogInfo();
            const std::vector<ServiceInfoPtr> &GetServiceInfos();
            const ServiceInfoPtr &GetServiceInfo(const std::string &protocol, const std::string &transport);
            bool ParseServiceInfo(const Json::Value &serviceObj);

            std::string name_;
            int32_t cpuStart_{0};
            int32_t threadNums_{1};
            int32_t cpus_{1};

        private:
            bool ParseLogInfo(const Json::Value &root);
            LogInfoPtr logInfo_;
            std::vector<ServiceInfoPtr> services_;
            std::mutex lock_;
        };
        using ConfigPtr = std::shared_ptr<Config>;

        class ConfigMgr: public NonCopyable
        {
        public:
            ConfigMgr() = default;
            ~ConfigMgr() = default;

            bool LoadConfig(const std::string &file);
            ConfigPtr GetConfig();
            
        private:
            ConfigPtr config_;
            std::mutex mtx_;
        };

        #define sConfigMgr nest::base::Singleton<nest::base::ConfigMgr>::Instance()
    }
}